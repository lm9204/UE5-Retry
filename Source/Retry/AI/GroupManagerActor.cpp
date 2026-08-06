#include "GroupManagerActor.h"

#include "AIController.h"
#include "AI/AreaControlEvaluator.h"
#include "AI/CommandExecutionMonitor.h"
#include "AI/OperationalTypes.h"
#include "AI/ReconMissionWorldAdapter.h"
#include "AI/SecureAreaWorldAdapter.h"
#include "AI/TeamOperationalMemorySubsystem.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenericTeamAgentInterface.h"
#include "LLMRequestQueue.h"
#include "RetryCharacter.h"
#include "RetryNPCCharacter.h"
#include "Components/HealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/NPCDecisionComponent.h"
#include "Components/PersonalityComponent.h"
#include "Scenario/ScenarioExecutionLogSubsystem.h"
#include "Scenario/ScenarioRuntimeSubsystem.h"

namespace GroupCommand
{
	FCommandAssignmentResult MakeResult(
		const ECommandAssignmentOutcome Outcome,
		const FString& Message)
	{
		FCommandAssignmentResult Result;
		Result.Outcome = Outcome;
		Result.Message = FText::FromString(Message);
		return Result;
	}

	FString BuildValidationMessage(
		const FCommandValidationResult& Validation)
	{
		TArray<FString> Messages;
		Messages.Reserve(Validation.Issues.Num());
		for (const FCommandValidationIssue& Issue : Validation.Issues)
		{
			Messages.Add(Issue.Message.ToString());
		}
		return FString::Join(Messages, TEXT(" | "));
	}

	void RecordRejection(
		UScenarioExecutionLogSubsystem* ExecutionLog,
		const FGuid& RunId,
		const FCommandIntent& Command,
		const FName GroupId,
		const FName ResultCode,
		const FString& Message)
	{
		ExecutionLog->RecordCommandEvent(
			RunId,
			Command.CommandId,
			GroupId,
			EScenarioExecutionEventType::CommandValidationRejected,
			ResultCode,
			Message);
	}

	FGroupMissionDispatchResult MakeDispatchResult(
		const EGroupMissionDispatchOutcome Outcome,
		const FString& Message,
		const int32 RecipientCount = 0)
	{
		FGroupMissionDispatchResult Result;
		Result.Outcome = Outcome;
		Result.Message = FText::FromString(Message);
		Result.RecipientCount = RecipientCount;
		return Result;
	}
}

AGroupManagerActor::AGroupManagerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

    // 렌더링/충돌 없는 순수 데이터·로직 액터
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}

void AGroupManagerActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateReconExecution();
	UpdateSecureExecution();
}

// ────────────────────────────────────────────────────
// 멤버 등록
// ────────────────────────────────────────────────────
void AGroupManagerActor::RegisterMember(
    ARetryNPCCharacter* NPC, bool bIsLeader)
{
    if (!NPC) return;

    if (bIsLeader)
    {
        Leader = NPC;
        UE_LOG(LogTemp, Warning,
            TEXT("[Group:%s] 리더 등록: %s"), *GroupID, *NPC->GetName());
    }

    Members.AddUnique(NPC);

    UE_LOG(LogTemp, Warning,
        TEXT("[Group:%s] 멤버 등록: %s (총 %d명)"),
        *GroupID, *NPC->GetName(), Members.Num());
}

// ────────────────────────────────────────────────────
// 그룹 메모리 기록 (목격자 기반)
// ────────────────────────────────────────────────────
void AGroupManagerActor::AddGroupMemory(
    const FString& WitnessID, const FString& EventType,
    FVector Location, float EmotionWeight, const FString& Description)
{
    FGroupMemoryEvent Event;
    Event.WitnessID = WitnessID;
    Event.EventType = EventType;
    Event.Location = Location;
    Event.Timestamp = GetWorld()->GetTimeSeconds();
    Event.EmotionWeight = EmotionWeight;
    Event.Description = Description;

    GroupMemories.Add(Event);
    AccumulatedEmotionScore += EmotionWeight;

    UE_LOG(LogTemp, Warning,
        TEXT("[Group:%s] %s 기록 (목격자: %s, Score: %.2f/%.2f)"),
        *GroupID, *Description, *WitnessID,
        AccumulatedEmotionScore, GroupEmotionThreshold);

    if (AccumulatedEmotionScore >= GroupEmotionThreshold)
    {
        UGameInstance* GI = GetGameInstance();
        if (GI)
        {
            if (ULLMRequestQueue* Queue = GI->GetSubsystem<ULLMRequestQueue>())
            {
                if (Queue->IsRequestEnabledForCurrentContext())
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("[Group:%s] 임계값 초과 — LLM 요청"), *GroupID);
                    Queue->EnqueueGroupRequest(this);
                }
            }
        }

        AccumulatedEmotionScore = 0.f;
    }
}

TArray<FGroupMemoryEvent> AGroupManagerActor::GetRecentGroupMemories(
    int32 Count) const
{
    TArray<FGroupMemoryEvent> Result;
    int32 Start = FMath::Max(0, GroupMemories.Num() - Count);
    for (int32 i = Start; i < GroupMemories.Num(); i++)
        Result.Add(GroupMemories[i]);
    return Result;
}

// ────────────────────────────────────────────────────
// 리더 사망 처리
// ────────────────────────────────────────────────────
void AGroupManagerActor::OnLeaderDied()
{
    UE_LOG(LogTemp, Warning,
        TEXT("[Group:%s] 리더 사망 — 그룹 통제 불능"), *GroupID);

	if (bHasCurrentCommand && !IsCommandStatusTerminal(CurrentCommand.Status))
	{
		CancelCurrentCommand(TEXT("LeaderDied"));
	}

	Leader = nullptr;
	ClearMissionForAllMembers();

    // 남은 부하 전원 즉시 명령 해제 → 개별 DecisionComponent 판단으로 전환
    SetOrderForAll(ENPCOrder::HoldFire, 0.f);
}

// ────────────────────────────────────────────────────
// 전체 멤버에게 명령 전파
// ────────────────────────────────────────────────────
void AGroupManagerActor::SetOrderForAll(ENPCOrder Order, float Weight)
{
    for (ARetryNPCCharacter* Member : Members)
    {
        if (!IsValid(Member)) continue;

        if (AAIController* AIC = Cast<AAIController>(Member->GetController()))
        {
            if (UNPCDecisionComponent* DC =
                AIC->FindComponentByClass<UNPCDecisionComponent>())
            {
                DC->SetOrder(Order, Weight);  // 아래 추가
            }
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[Group:%s] 전체 명령 전파: Weight=%.2f (%d명)"),
        *GroupID, Weight, Members.Num());
}

FCommandAssignmentResult AGroupManagerActor::AssignCommand(
	const FCommandIntent& Command)
{
	UGameInstance* GameInstance = GetGameInstance();
	UScenarioRuntimeSubsystem* Runtime = GameInstance
		? GameInstance->GetSubsystem<UScenarioRuntimeSubsystem>()
		: nullptr;
	if (!Runtime)
	{
		return GroupCommand::MakeResult(
			ECommandAssignmentOutcome::NoActiveScenario,
			TEXT("A Scenario run is required before assigning a command."));
	}

	const FScenarioRunContext RunContext = Runtime->GetCurrentRunContext();
	if (!RunContext.IsValid())
	{
		return GroupCommand::MakeResult(
			ECommandAssignmentOutcome::NoActiveScenario,
			TEXT("A Scenario run is required before assigning a command."));
	}

	UScenarioExecutionLogSubsystem* ExecutionLog =
		GameInstance->GetSubsystem<UScenarioExecutionLogSubsystem>();
	return AssignCommandForRun(Command, RunContext.RunId, ExecutionLog);
}

FCommandAssignmentResult AGroupManagerActor::AssignCommandForRun(
	const FCommandIntent& Command,
	const FGuid& RunId,
	UScenarioExecutionLogSubsystem* ExecutionLog)
{
	if (!ExecutionLog || !ExecutionLog->IsRecordingRun(RunId))
	{
		return GroupCommand::MakeResult(
			ECommandAssignmentOutcome::ExecutionLogUnavailable,
			TEXT("The active Scenario run is not available for command logging."));
	}

	const FName CommandGroupId = GetCommandGroupId();
	if (CommandGroupId.IsNone())
	{
		const FString Message = TEXT("The Group Manager requires a Group ID.");
		GroupCommand::RecordRejection(
			ExecutionLog, RunId, Command, NAME_None,
			TEXT("InvalidGroupConfiguration"), Message);
		return GroupCommand::MakeResult(
			ECommandAssignmentOutcome::InvalidGroupConfiguration, Message);
	}

	if (bHasCurrentCommand && !IsCommandStatusTerminal(CurrentCommand.Status))
	{
		const FString Message = TEXT(
			"The current non-terminal command must finish or be cancelled first.");
		GroupCommand::RecordRejection(
			ExecutionLog, RunId, Command, CommandGroupId,
			TEXT("ActiveCommandExists"), Message);
		return GroupCommand::MakeResult(
			ECommandAssignmentOutcome::ActiveCommandExists, Message);
	}

	const FCommandValidationResult Validation =
		FCommandValidator::Validate(Command);
	if (!Validation.IsValid())
	{
		const FString Message = GroupCommand::BuildValidationMessage(Validation);
		GroupCommand::RecordRejection(
			ExecutionLog, RunId, Command, CommandGroupId,
			TEXT("ValidationRejected"), Message);
		FCommandAssignmentResult Result = GroupCommand::MakeResult(
			ECommandAssignmentOutcome::ValidationRejected, Message);
		Result.Validation = Validation;
		return Result;
	}

	if (Command.AssignedGroupId != CommandGroupId)
	{
		const FString Message = FString::Printf(
			TEXT("Command group '%s' does not match Group Manager '%s'."),
			*Command.AssignedGroupId.ToString(), *CommandGroupId.ToString());
		GroupCommand::RecordRejection(
			ExecutionLog, RunId, Command, CommandGroupId,
			TEXT("GroupMismatch"), Message);
		return GroupCommand::MakeResult(
			ECommandAssignmentOutcome::GroupMismatch, Message);
	}

	if (!ExecutionLog->RecordCommandEvent(
		RunId,
		Command.CommandId,
		CommandGroupId,
		EScenarioExecutionEventType::CommandValidated,
		TEXT("ValidationSucceeded"),
		TEXT("Command validation succeeded.")).IsValid())
	{
		return GroupCommand::MakeResult(
			ECommandAssignmentOutcome::ExecutionLogUnavailable,
			TEXT("Command validation could not be recorded."));
	}

	FCommandIntent AssignedCommand = Command;
	const ECommandStatus AssignmentStatuses[] =
	{
		ECommandStatus::Validated,
		ECommandStatus::Assigned,
	};

	for (const ECommandStatus NewStatus : AssignmentStatuses)
	{
		const ECommandStatus PreviousStatus = AssignedCommand.Status;
		FText TransitionError;
		if (!TryTransitionCommandStatus(
			AssignedCommand, NewStatus, TransitionError))
		{
			return GroupCommand::MakeResult(
				ECommandAssignmentOutcome::TransitionRejected,
				TransitionError.ToString());
		}

		if (!ExecutionLog->RecordCommandStatusTransition(
			RunId,
			AssignedCommand.CommandId,
			CommandGroupId,
			PreviousStatus,
			NewStatus,
			TEXT("AssignmentProgressed"),
			TEXT("Command assignment status progressed.")).IsValid())
		{
			return GroupCommand::MakeResult(
				ECommandAssignmentOutcome::ExecutionLogUnavailable,
				TEXT("Command assignment status could not be recorded."));
		}
	}

	ForceClearCurrentCommand();
	CurrentCommand = MoveTemp(AssignedCommand);
	bHasCurrentCommand = true;
	return GroupCommand::MakeResult(
		ECommandAssignmentOutcome::Assigned,
		TEXT("Command validated and assigned."));
}

bool AGroupManagerActor::TransitionCurrentCommandStatus(
	const ECommandStatus NewStatus,
	const FName ResultCode,
	const FString& Message)
{
	UGameInstance* GameInstance = GetGameInstance();
	UScenarioRuntimeSubsystem* Runtime = GameInstance
		? GameInstance->GetSubsystem<UScenarioRuntimeSubsystem>()
		: nullptr;
	if (!Runtime)
	{
		return false;
	}

	const FScenarioRunContext RunContext = Runtime->GetCurrentRunContext();
	UScenarioExecutionLogSubsystem* ExecutionLog = RunContext.IsValid()
		? GameInstance->GetSubsystem<UScenarioExecutionLogSubsystem>()
		: nullptr;
	return TransitionCurrentCommandStatusForRun(
		NewStatus, ResultCode, Message, RunContext.RunId, ExecutionLog);
}

bool AGroupManagerActor::TransitionCurrentCommandStatusForRun(
	const ECommandStatus NewStatus,
	const FName ResultCode,
	const FString& Message,
	const FGuid& RunId,
	UScenarioExecutionLogSubsystem* ExecutionLog)
{
	if (!bHasCurrentCommand
		|| !CanTransitionCommandStatus(CurrentCommand.Status, NewStatus)
		|| !ExecutionLog
		|| !ExecutionLog->IsRecordingRun(RunId))
	{
		return false;
	}

	const ECommandStatus PreviousStatus = CurrentCommand.Status;
	const FName EffectiveResultCode = ResultCode.IsNone()
		? FName(TEXT("StatusChanged"))
		: ResultCode;
	if (!ExecutionLog->RecordCommandStatusTransition(
		RunId,
		CurrentCommand.CommandId,
		GetCommandGroupId(),
		PreviousStatus,
		NewStatus,
		EffectiveResultCode,
		Message).IsValid())
	{
		return false;
	}

	FText TransitionError;
	const bool bTransitioned = TryTransitionCommandStatus(
		CurrentCommand, NewStatus, TransitionError);
	if (bTransitioned && IsCommandStatusTerminal(NewStatus))
	{
		ClearMissionForAllMembers();
	}
	return bTransitioned;
}

bool AGroupManagerActor::DispatchCurrentReconMission()
{
	UGameInstance* GameInstance = GetGameInstance();
	UScenarioRuntimeSubsystem* Runtime = GameInstance
		? GameInstance->GetSubsystem<UScenarioRuntimeSubsystem>()
		: nullptr;
	if (!Runtime)
	{
		return false;
	}

	const FScenarioRunContext RunContext = Runtime->GetCurrentRunContext();
	UScenarioExecutionLogSubsystem* ExecutionLog = RunContext.IsValid()
		? GameInstance->GetSubsystem<UScenarioExecutionLogSubsystem>()
		: nullptr;
	const FGroupMissionDispatchResult Result =
		DispatchCurrentReconMissionForRun(RunContext.RunId, ExecutionLog);
	UE_LOG(LogTemp, Log,
		TEXT("[Group:%s] Mission dispatch %s: %s"),
		*GroupID,
		Result.IsSuccess() ? TEXT("succeeded") : TEXT("failed"),
		*Result.Message.ToString());
	return Result.IsSuccess();
}

FGroupMissionDispatchResult
AGroupManagerActor::DispatchCurrentReconMissionForRun(
	const FGuid& RunId,
	UScenarioExecutionLogSubsystem* ExecutionLog)
{
	if (!bHasCurrentCommand)
	{
		return DispatchCurrentMissionForRun(RunId, ExecutionLog);
	}
	if (CurrentCommand.Verb != ECommandVerb::Recon)
	{
		return GroupCommand::MakeDispatchResult(
			EGroupMissionDispatchOutcome::InvalidCommandState,
			TEXT("The current command is not a Recon mission."));
	}
	return DispatchCurrentMissionForRun(RunId, ExecutionLog);
}

FGroupMissionDispatchResult AGroupManagerActor::DispatchCurrentMissionForRun(
	const FGuid& RunId,
	UScenarioExecutionLogSubsystem* ExecutionLog)
{
	if (!bHasCurrentCommand)
	{
		return GroupCommand::MakeDispatchResult(
			EGroupMissionDispatchOutcome::NoAssignedCommand,
			TEXT("The group has no assigned command to dispatch."));
	}

	if (CurrentCommand.Status != ECommandStatus::Assigned)
	{
		return GroupCommand::MakeDispatchResult(
			EGroupMissionDispatchOutcome::InvalidCommandState,
			TEXT("Only an Assigned command can be dispatched."));
	}

	if (!IsValid(Leader)
		|| !Leader->HealthComponent
		|| Leader->HealthComponent->IsDead())
	{
		return GroupCommand::MakeDispatchResult(
			EGroupMissionDispatchOutcome::LeaderUnavailable,
			TEXT("A living group leader is required to dispatch the mission."));
	}

	AAIController* LeaderController =
		Cast<AAIController>(Leader->GetController());
	if (!LeaderController
		|| !LeaderController->FindComponentByClass<UNPCDecisionComponent>())
	{
		return GroupCommand::MakeDispatchResult(
			EGroupMissionDispatchOutcome::LeaderUnavailable,
			TEXT("The group leader cannot receive a mission."));
	}

	TArray<UNPCDecisionComponent*> Recipients;
	for (ARetryNPCCharacter* Member : Members)
	{
		if (!IsValid(Member))
		{
			continue;
		}

		if (!Member->HealthComponent)
		{
			return GroupCommand::MakeDispatchResult(
				EGroupMissionDispatchOutcome::RecipientUnavailable,
				FString::Printf(
					TEXT("Living state is unavailable for member '%s'."),
					*Member->GetName()));
		}

		if (Member->HealthComponent->IsDead())
		{
			continue;
		}

		AAIController* MemberController =
			Cast<AAIController>(Member->GetController());
		UNPCDecisionComponent* Decision = MemberController
			? MemberController->FindComponentByClass<UNPCDecisionComponent>()
			: nullptr;
		if (!Decision)
		{
			return GroupCommand::MakeDispatchResult(
				EGroupMissionDispatchOutcome::RecipientUnavailable,
				FString::Printf(
					TEXT("Living member '%s' cannot receive the mission."),
					*Member->GetName()));
		}

		Recipients.AddUnique(Decision);
	}

	UNPCDecisionComponent* LeaderDecision =
		LeaderController->FindComponentByClass<UNPCDecisionComponent>();
	if (!Recipients.Contains(LeaderDecision))
	{
		return GroupCommand::MakeDispatchResult(
			EGroupMissionDispatchOutcome::LeaderUnavailable,
			TEXT("The living leader is not registered as a group member."));
	}

	FMissionContext Mission;
	float ObjectiveAreaRadius = 0.f;
	if (CurrentCommand.Verb == ECommandVerb::Recon)
	{
		const FReconMissionWorldResult WorldResult =
			FReconMissionWorldAdapter::Resolve(
				GetWorld(), CurrentCommand,
				Leader->GetActorLocation(), Leader);
		if (!WorldResult.IsSuccess())
		{
			return GroupCommand::MakeDispatchResult(
				EGroupMissionDispatchOutcome::WorldResolutionFailed,
				TEXT("The assigned Recon mission could not be resolved in the World."),
				Recipients.Num());
		}
		Mission = WorldResult.Resolution.Mission;
		UE_LOG(LogTemp, Display,
			TEXT("[Group:%s] Recon Mission resolved. Observation:%s Location:%s Candidates:%d"),
			*GroupID,
			*Mission.ObjectiveId.ToString(),
			*Mission.ObjectiveLocation.ToCompactString(),
			WorldResult.CandidateCount);
	}
	else if (CurrentCommand.Verb == ECommandVerb::Secure)
	{
		const FSecureAreaWorldResult WorldResult =
			FSecureAreaWorldAdapter::Resolve(
				GetWorld(), CurrentCommand,
				Leader->GetActorLocation(), Leader);
		if (!WorldResult.IsSuccess())
		{
			return GroupCommand::MakeDispatchResult(
				EGroupMissionDispatchOutcome::WorldResolutionFailed,
				TEXT("The assigned Secure Area mission could not be resolved in the World."),
				Recipients.Num());
		}
		Mission = WorldResult.Resolution.Mission;
		ObjectiveAreaRadius = WorldResult.AreaRadius;
		UE_LOG(LogTemp, Display,
			TEXT("[Group:%s] Secure Mission resolved. Area:%s Location:%s Radius:%.1f"),
			*GroupID,
			*Mission.ObjectiveId.ToString(),
			*Mission.ObjectiveLocation.ToCompactString(),
			ObjectiveAreaRadius);
	}
	else
	{
		return GroupCommand::MakeDispatchResult(
			EGroupMissionDispatchOutcome::WorldResolutionFailed,
			TEXT("The current command has no Mission World resolver."),
			Recipients.Num());
	}

	return DispatchResolvedMissionForRun(
		Mission, Recipients, RunId, ExecutionLog, ObjectiveAreaRadius);
}

FGroupMissionDispatchResult
AGroupManagerActor::DispatchResolvedMissionForRun(
	const FMissionContext& Mission,
	const TArray<UNPCDecisionComponent*>& Recipients,
	const FGuid& RunId,
	UScenarioExecutionLogSubsystem* ExecutionLog,
	const float ObjectiveAreaRadius)
{
	if (!bHasCurrentCommand)
	{
		return GroupCommand::MakeDispatchResult(
			EGroupMissionDispatchOutcome::NoAssignedCommand,
			TEXT("The group has no assigned command to dispatch."));
	}

	if (CurrentCommand.Status != ECommandStatus::Assigned
		|| Mission.CommandId != CurrentCommand.CommandId)
	{
		return GroupCommand::MakeDispatchResult(
			EGroupMissionDispatchOutcome::InvalidCommandState,
			TEXT("The Mission does not match the current Assigned command."));
	}

	if (CurrentCommand.Verb == ECommandVerb::Secure
		&& (!FMath::IsFinite(ObjectiveAreaRadius)
			|| ObjectiveAreaRadius <= 0.f))
	{
		return GroupCommand::MakeDispatchResult(
			EGroupMissionDispatchOutcome::MissionRejected,
			TEXT("A Secure Area Mission requires a finite positive Area radius."));
	}

	if (Recipients.IsEmpty())
	{
		return GroupCommand::MakeDispatchResult(
			EGroupMissionDispatchOutcome::RecipientUnavailable,
			TEXT("The mission requires at least one recipient."));
	}

	struct FMissionSnapshot
	{
		UNPCDecisionComponent* Decision = nullptr;
		bool bHadMission = false;
		FMissionContext Mission;
	};

	TArray<FMissionSnapshot> Snapshots;
	Snapshots.Reserve(Recipients.Num());
	TSet<UNPCDecisionComponent*> UniqueRecipients;
	for (UNPCDecisionComponent* Decision : Recipients)
	{
		if (!IsValid(Decision) || UniqueRecipients.Contains(Decision))
		{
			return GroupCommand::MakeDispatchResult(
				EGroupMissionDispatchOutcome::RecipientUnavailable,
				TEXT("Every mission recipient must be valid and unique."));
		}

		UniqueRecipients.Add(Decision);
		FMissionSnapshot& Snapshot = Snapshots.AddDefaulted_GetRef();
		Snapshot.Decision = Decision;
		Snapshot.bHadMission = Decision->HasActiveMission();
		if (Snapshot.bHadMission)
		{
			Snapshot.Mission = Decision->GetActiveMissionContext();
		}
	}

	const auto RestoreSnapshots = [&Snapshots]()
	{
		for (const FMissionSnapshot& Snapshot : Snapshots)
		{
			if (Snapshot.bHadMission)
			{
				Snapshot.Decision->SetMissionContext(Snapshot.Mission);
			}
			else
			{
				Snapshot.Decision->ClearMissionContext();
			}
		}
	};

	for (const FMissionSnapshot& Snapshot : Snapshots)
	{
		if (!Snapshot.Decision->SetMissionContext(Mission))
		{
			RestoreSnapshots();
			return GroupCommand::MakeDispatchResult(
				EGroupMissionDispatchOutcome::MissionRejected,
				TEXT("A recipient rejected the Mission; all recipients were restored."),
				Snapshots.Num());
		}
	}

	if (!TransitionCurrentCommandStatusForRun(
		ECommandStatus::Executing,
		TEXT("MissionDispatched"),
		TEXT("Mission was atomically dispatched to all living group members."),
		RunId,
		ExecutionLog))
	{
		RestoreSnapshots();
		return GroupCommand::MakeDispatchResult(
			EGroupMissionDispatchOutcome::StatusTransitionFailed,
			TEXT("Command status could not advance; all recipients were restored."),
			Snapshots.Num());
	}

	ActiveMissionRecipients.Reset(Snapshots.Num());
	for (const FMissionSnapshot& Snapshot : Snapshots)
	{
		ActiveMissionRecipients.Add(Snapshot.Decision);
	}
	if (CurrentCommand.Verb == ECommandVerb::Recon)
	{
		BeginReconMonitoring(Mission, RunId, ExecutionLog);
	}
	else if (CurrentCommand.Verb == ECommandVerb::Secure)
	{
		BeginSecureMonitoring(
			Mission, ObjectiveAreaRadius, RunId, ExecutionLog);
	}

	return GroupCommand::MakeDispatchResult(
		EGroupMissionDispatchOutcome::Dispatched,
		TEXT("Mission dispatched to all living group members."),
		Snapshots.Num());
}

bool AGroupManagerActor::CancelCurrentCommand(const FName ReasonCode)
{
	const FName EffectiveReason = ReasonCode.IsNone()
		? FName(TEXT("CancelledByIssuer"))
		: ReasonCode;
	return TransitionCurrentCommandStatus(
		ECommandStatus::Cancelled,
		EffectiveReason,
		TEXT("Command cancelled."));
}

bool AGroupManagerActor::ClearCurrentCommand()
{
	if (!bHasCurrentCommand)
	{
		return true;
	}

	if (!IsCommandStatusTerminal(CurrentCommand.Status))
	{
		return false;
	}

	ClearMissionForAllMembers();
	ForceClearCurrentCommand();
	return true;
}

bool AGroupManagerActor::HasCurrentCommand() const
{
	return bHasCurrentCommand;
}

FCommandIntent AGroupManagerActor::GetCurrentCommand() const
{
	return bHasCurrentCommand ? CurrentCommand : FCommandIntent();
}

void AGroupManagerActor::ResetGroupRuntimeState()
{
    GroupMemories.Reset();
	AccumulatedEmotionScore = 0.f;
	ClearMissionForAllMembers();
	ForceClearCurrentCommand();

    // Leader and Members describe the placed level configuration. They may already
    // have been registered depending on actor BeginPlay order, so preserve them.
    SetOrderForAll(ENPCOrder::HoldFire, 0.f);
}

FName AGroupManagerActor::GetCommandGroupId() const
{
	return FName(*GroupID.TrimStartAndEnd());
}

void AGroupManagerActor::ClearMissionForAllMembers()
{
	for (const TWeakObjectPtr<UNPCDecisionComponent>& Recipient :
		ActiveMissionRecipients)
	{
		if (UNPCDecisionComponent* Decision = Recipient.Get())
		{
			Decision->ClearMissionContext();
		}
	}
	ActiveMissionRecipients.Reset();

	for (ARetryNPCCharacter* Member : Members)
	{
		if (!IsValid(Member))
		{
			continue;
		}

		if (AAIController* Controller =
			Cast<AAIController>(Member->GetController()))
		{
			if (UNPCDecisionComponent* Decision =
				Controller->FindComponentByClass<UNPCDecisionComponent>())
			{
				Decision->ClearMissionContext();
			}
		}
	}

	StopReconMonitoring();
	StopSecureMonitoring();
}

void AGroupManagerActor::BeginReconMonitoring(
	const FMissionContext& Mission,
	const FGuid& RunId,
	UScenarioExecutionLogSubsystem* ExecutionLog)
{
	ActiveReconMission = Mission;
	ActiveReconRunId = RunId;
	ActiveReconExecutionLog = ExecutionLog;
	ReconExecutionStartedAtSeconds = GetWorld()
		? static_cast<double>(GetWorld()->GetTimeSeconds())
		: 0.0;
	ReconObservationStartedAtSeconds = -1.0;
	bReconMonitoringActive = true;
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Display,
		TEXT("[Group:%s] Recon monitoring started. Observation:%s Location:%s Radius:%.1f"),
		*GroupID,
		*Mission.ObjectiveId.ToString(),
		*Mission.ObjectiveLocation.ToCompactString(),
		ReconObservationArrivalRadius);
}

void AGroupManagerActor::UpdateReconExecution()
{
	if (!bReconMonitoringActive
		|| !bHasCurrentCommand
		|| CurrentCommand.Status != ECommandStatus::Executing
		|| CurrentCommand.CommandId != ActiveReconMission.CommandId
		|| !GetWorld())
	{
		return;
	}

	const double NowSeconds =
		static_cast<double>(GetWorld()->GetTimeSeconds());
	const bool bLeaderAvailable = IsValid(Leader)
		&& Leader->HealthComponent
		&& !Leader->HealthComponent->IsDead();

	UNPCDecisionComponent* LeaderDecision = nullptr;
	if (bLeaderAvailable)
	{
		if (AAIController* Controller =
			Cast<AAIController>(Leader->GetController()))
		{
			LeaderDecision =
				Controller->FindComponentByClass<UNPCDecisionComponent>();
		}
	}

	const float VerticalTolerance = bLeaderAvailable
		&& Leader->GetCapsuleComponent()
		? Leader->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
			+ ReconObservationArrivalRadius
		: ReconObservationArrivalRadius;
	const bool bAtObservationPoint = bLeaderAvailable
		&& FCommandExecutionMonitor::IsWithinObservationRange(
			Leader->GetActorLocation(),
			ActiveReconMission.ObjectiveLocation,
			ReconObservationArrivalRadius,
			VerticalTolerance);
	const bool bObservationAllowed = LeaderDecision
		&& LeaderDecision->HasActiveMission()
		&& LeaderDecision->IsMissionMovementAllowedForState(
			LeaderDecision->GetCombatState());

	if (bAtObservationPoint && bObservationAllowed)
	{
		if (ReconObservationStartedAtSeconds < 0.0)
		{
			ReconObservationStartedAtSeconds = NowSeconds;
			UE_LOG(LogTemp, Display,
				TEXT("[Group:%s] Recon observation started. Hold:%.1fs HorizontalDistance:%.1f VerticalDistance:%.1f"),
				*GroupID,
				CurrentCommand.CompletionCriteria.MinimumHoldSeconds,
				FVector::Dist2D(
					Leader->GetActorLocation(),
					ActiveReconMission.ObjectiveLocation),
				FMath::Abs(
					Leader->GetActorLocation().Z
					- ActiveReconMission.ObjectiveLocation.Z));
		}
	}
	else
	{
		if (ReconObservationStartedAtSeconds >= 0.0)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[Group:%s] Recon observation interrupted. AtPoint:%s Allowed:%s"),
				*GroupID,
				bAtObservationPoint ? TEXT("true") : TEXT("false"),
				bObservationAllowed ? TEXT("true") : TEXT("false"));
		}
		ReconObservationStartedAtSeconds = -1.0;
	}

	FReconExecutionSnapshot Snapshot;
	Snapshot.bLeaderAvailable = bLeaderAvailable;
	Snapshot.bAtObservationPoint = bAtObservationPoint;
	Snapshot.bObservationAllowed = bObservationAllowed;
	Snapshot.ExecutionElapsedSeconds =
		FMath::Max(0.0, NowSeconds - ReconExecutionStartedAtSeconds);
	Snapshot.StableObservationSeconds =
		ReconObservationStartedAtSeconds >= 0.0
			? FMath::Max(0.0, NowSeconds - ReconObservationStartedAtSeconds)
			: 0.0;

	const FReconExecutionDecision Decision =
		FCommandExecutionMonitor::EvaluateRecon(CurrentCommand, Snapshot);
	switch (Decision.Outcome)
	{
	case EReconExecutionOutcome::ObservationReady:
		SubmitReconReportAndComplete(NowSeconds);
		break;
	case EReconExecutionOutcome::FailedLeaderUnavailable:
		TransitionCurrentCommandStatusForRun(
			ECommandStatus::Failed,
			TEXT("LeaderUnavailable"),
			TEXT("Recon failed because the group leader is unavailable."),
			ActiveReconRunId,
			ActiveReconExecutionLog.Get());
		break;
	case EReconExecutionOutcome::FailedTimeout:
		TransitionCurrentCommandStatusForRun(
			ECommandStatus::Failed,
			TEXT("ReconTimeout"),
			TEXT("Recon failed because the command timeout elapsed."),
			ActiveReconRunId,
			ActiveReconExecutionLog.Get());
		break;
	default:
		break;
	}
}

bool AGroupManagerActor::SubmitReconReportAndComplete(
	const double ObservedAtSeconds)
{
	UScenarioExecutionLogSubsystem* ExecutionLog =
		ActiveReconExecutionLog.Get();
	if (!ExecutionLog
		|| !ExecutionLog->IsRecordingRun(ActiveReconRunId))
	{
		return false;
	}

	FOperationalReport Report;
	FText ReportError;
	if (!BuildReconOperationalReport(
		CurrentCommand,
		ActiveReconMission,
		ActiveReconRunId,
		TeamID,
		GetCommandGroupId(),
		ObservedAtSeconds,
		Report,
		ReportError))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Group:%s] Recon Report build failed: %s"),
			*GroupID,
			*ReportError.ToString());
		TransitionCurrentCommandStatusForRun(
			ECommandStatus::Failed,
			TEXT("ReconReportBuildFailed"),
			ReportError.ToString(),
			ActiveReconRunId,
			ExecutionLog);
		return false;
	}

	return SubmitBuiltReportAndComplete(
		Report,
		ActiveReconRunId,
		ExecutionLog,
		TEXT("ReconObservation"),
		TEXT("ReconReportCreated"),
		TEXT("ReconReportReceived"),
		TEXT("Recon"));
}

bool AGroupManagerActor::SubmitBuiltReportAndComplete(
	FOperationalReport& Report,
	const FGuid& RunId,
	UScenarioExecutionLogSubsystem* ExecutionLog,
	const FName FactResultCode,
	const FName CreatedResultCode,
	const FName ReceivedResultCode,
	const FString& OperationLabel)
{
	for (const FOperationalFact& Fact : Report.Facts)
	{
		if (!ExecutionLog->RecordOperationalEvent(
			RunId,
			CurrentCommand.CommandId,
			GetCommandGroupId(),
			EScenarioExecutionEventType::OperationalFactObserved,
			Fact.FactId,
			Report.ReportId,
			FactResultCode,
			FString::Printf(TEXT("A %s operational Fact was confirmed."),
				*OperationLabel)).IsValid())
		{
			return false;
		}
	}

	if (!ExecutionLog->RecordOperationalEvent(
		RunId,
		CurrentCommand.CommandId,
		GetCommandGroupId(),
		EScenarioExecutionEventType::OperationalReportCreated,
		FGuid(),
		Report.ReportId,
		CreatedResultCode,
		FString::Printf(TEXT("A %s report was created."),
			*OperationLabel)).IsValid())
	{
		return false;
	}

	UTeamOperationalMemorySubsystem* TeamMemory = GetWorld()
		? GetWorld()->GetSubsystem<UTeamOperationalMemorySubsystem>()
		: nullptr;
	if (!TeamMemory)
	{
		return false;
	}

	FOperationalReport ReceivedReport;
	FText ReportError;
	if (!TeamMemory->ReceiveReport(
		Report, ReceivedReport, ReportError))
	{
		return false;
	}

	if (!ExecutionLog->RecordOperationalEvent(
		RunId,
		CurrentCommand.CommandId,
		GetCommandGroupId(),
		EScenarioExecutionEventType::OperationalReportReceived,
		FGuid(),
		ReceivedReport.ReportId,
		ReceivedResultCode,
		FString::Printf(TEXT("HQ received the %s report."),
			*OperationLabel)).IsValid())
	{
		return false;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[TeamMemory:%u] %s Report received. Report:%s Command:%s Facts:%d"),
		TeamID,
		*OperationLabel,
		*ReceivedReport.ReportId.ToString(EGuidFormats::DigitsWithHyphens),
		*CurrentCommand.CommandId.ToString(EGuidFormats::DigitsWithHyphens),
		ReceivedReport.Facts.Num());

	for (const FInformationRequirement& Requirement
		: CurrentCommand.InformationRequirements)
	{
		if (Requirement.bRequired
			&& !TeamMemory->HasReceivedRequirement(
				TeamID,
				RunId,
				CurrentCommand.CommandId,
				Requirement))
		{
			return false;
		}
	}

	const FGuid CompletedCommandId = CurrentCommand.CommandId;
	if (!TransitionCurrentCommandStatusForRun(
		ECommandStatus::Completed,
		ReceivedResultCode,
		FString::Printf(TEXT("All required %s information was received."),
			*OperationLabel),
		RunId,
		ExecutionLog))
	{
		return false;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[Group:%s] %s completed. Command:%s Report:%s Facts:%d"),
		*GroupID,
		*OperationLabel,
		*CompletedCommandId.ToString(EGuidFormats::DigitsWithHyphens),
		*ReceivedReport.ReportId.ToString(EGuidFormats::DigitsWithHyphens),
		ReceivedReport.Facts.Num());
	return true;
}

void AGroupManagerActor::StopReconMonitoring()
{
	bReconMonitoringActive = false;
	ActiveReconMission = FMissionContext();
	ActiveReconRunId.Invalidate();
	ActiveReconExecutionLog.Reset();
	ReconExecutionStartedAtSeconds = 0.0;
	ReconObservationStartedAtSeconds = -1.0;
	SetActorTickEnabled(false);
}

void AGroupManagerActor::BeginSecureMonitoring(
	const FMissionContext& Mission,
	const float AreaRadius,
	const FGuid& RunId,
	UScenarioExecutionLogSubsystem* ExecutionLog)
{
	ActiveSecureMission = Mission;
	ActiveSecureAreaRadius = AreaRadius;
	ActiveSecureRunId = RunId;
	ActiveSecureExecutionLog = ExecutionLog;
	SecureExecutionStartedAtSeconds = GetWorld()
		? static_cast<double>(GetWorld()->GetTimeSeconds())
		: 0.0;
	SecureControlStartedAtSeconds = -1.0;
	bSecureMonitoringActive = true;
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Display,
		TEXT("[Group:%s] Secure monitoring started. Area:%s Radius:%.1f"),
		*GroupID,
		*Mission.ObjectiveId.ToString(),
		AreaRadius);
}

void AGroupManagerActor::UpdateSecureExecution()
{
	if (!bSecureMonitoringActive
		|| !bHasCurrentCommand
		|| CurrentCommand.Status != ECommandStatus::Executing
		|| CurrentCommand.CommandId != ActiveSecureMission.CommandId
		|| !GetWorld())
	{
		return;
	}

	const double NowSeconds =
		static_cast<double>(GetWorld()->GetTimeSeconds());
	const auto IsLiving = [](const ACharacter* Character)
	{
		const UHealthComponent* Health = nullptr;
		if (const ARetryNPCCharacter* NPC =
			Cast<ARetryNPCCharacter>(Character))
		{
			Health = NPC->HealthComponent;
		}
		else if (const ARetryCharacter* Player =
			Cast<ARetryCharacter>(Character))
		{
			Health = Player->HealthComponent;
		}
		return IsValid(Character) && Health && !Health->IsDead();
	};
	const auto IsInsideArea = [this](const ACharacter* Character)
	{
		if (!IsValid(Character) || !Character->GetCapsuleComponent())
		{
			return false;
		}
		const FVector Location = Character->GetActorLocation();
		const FVector AreaLocation = ActiveSecureMission.ObjectiveLocation;
		const float VerticalTolerance =
			Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 100.f;
		return FVector::DistSquared2D(Location, AreaLocation)
				<= FMath::Square(ActiveSecureAreaRadius)
			&& FMath::Abs(Location.Z - AreaLocation.Z)
				<= VerticalTolerance;
	};

	int32 LivingGroupMemberCount = 0;
	for (const ARetryNPCCharacter* Member : Members)
	{
		if (IsLiving(Member))
		{
			++LivingGroupMemberCount;
		}
	}

	int32 FriendlyCountInside = 0;
	int32 HostileCountInside = 0;
	const IGenericTeamAgentInterface* LeaderTeamAgent = Leader
		? Cast<IGenericTeamAgentInterface>(Leader->GetController())
		: nullptr;
	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
	{
		ACharacter* Character = *It;
		if (!IsLiving(Character) || !IsInsideArea(Character))
		{
			continue;
		}
		const ETeamAttitude::Type Attitude = LeaderTeamAgent
			? LeaderTeamAgent->GetTeamAttitudeTowards(*Character)
			: ETeamAttitude::Neutral;
		if (Attitude == ETeamAttitude::Friendly)
		{
			++FriendlyCountInside;
		}
		else if (Attitude == ETeamAttitude::Hostile)
		{
			++HostileCountInside;
		}
	}

	const bool bLeaderAvailable = IsLiving(Leader);
	const bool bLeaderInsideArea =
		bLeaderAvailable && IsInsideArea(Leader);
	const bool bHasControl = bLeaderInsideArea
		&& FriendlyCountInside > 0
		&& HostileCountInside == 0;
	if (bHasControl)
	{
		if (SecureControlStartedAtSeconds < 0.0)
		{
			SecureControlStartedAtSeconds = NowSeconds;
			UE_LOG(LogTemp, Display,
				TEXT("[Group:%s] Secure control hold started. Hold:%.1fs Friendlies:%d"),
				*GroupID,
				CurrentCommand.CompletionCriteria.MinimumHoldSeconds,
				FriendlyCountInside);
		}
	}
	else
	{
		if (SecureControlStartedAtSeconds >= 0.0)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[Group:%s] Secure control hold interrupted. LeaderInside:%s Hostiles:%d"),
				*GroupID,
				bLeaderInsideArea ? TEXT("true") : TEXT("false"),
				HostileCountInside);
		}
		SecureControlStartedAtSeconds = -1.0;
	}

	FAreaControlSnapshot Snapshot;
	Snapshot.bLeaderAvailable = bLeaderAvailable;
	Snapshot.bLeaderInsideArea = bLeaderInsideArea;
	Snapshot.LivingGroupMemberCount = LivingGroupMemberCount;
	Snapshot.FriendlyCountInside = FriendlyCountInside;
	Snapshot.HostileCountInside = HostileCountInside;
	Snapshot.ExecutionElapsedSeconds =
		FMath::Max(0.0, NowSeconds - SecureExecutionStartedAtSeconds);
	Snapshot.StableControlSeconds =
		SecureControlStartedAtSeconds >= 0.0
			? FMath::Max(0.0, NowSeconds - SecureControlStartedAtSeconds)
			: 0.0;

	const FAreaControlDecision Decision =
		FAreaControlEvaluator::EvaluateSecureArea(CurrentCommand, Snapshot);
	switch (Decision.Outcome)
	{
	case EAreaControlOutcome::Secured:
		SubmitSecureReportAndComplete(NowSeconds);
		break;
	case EAreaControlOutcome::FailedLeaderUnavailable:
		TransitionCurrentCommandStatusForRun(
			ECommandStatus::Failed,
			TEXT("LeaderUnavailable"),
			TEXT("Secure Area failed because the group leader is unavailable."),
			ActiveSecureRunId,
			ActiveSecureExecutionLog.Get());
		break;
	case EAreaControlOutcome::FailedNoCombatPower:
		TransitionCurrentCommandStatusForRun(
			ECommandStatus::Failed,
			TEXT("NoCombatPower"),
			TEXT("Secure Area failed because the group has no living combat power."),
			ActiveSecureRunId,
			ActiveSecureExecutionLog.Get());
		break;
	case EAreaControlOutcome::FailedTimeout:
		TransitionCurrentCommandStatusForRun(
			ECommandStatus::Failed,
			TEXT("SecureAreaTimeout"),
			TEXT("Secure Area failed because the command timeout elapsed."),
			ActiveSecureRunId,
			ActiveSecureExecutionLog.Get());
		break;
	default:
		break;
	}
}

bool AGroupManagerActor::SubmitSecureReportAndComplete(
	const double SecuredAtSeconds)
{
	UScenarioExecutionLogSubsystem* ExecutionLog =
		ActiveSecureExecutionLog.Get();
	if (!ExecutionLog
		|| !ExecutionLog->IsRecordingRun(ActiveSecureRunId))
	{
		return false;
	}

	FOperationalReport Report;
	FText ReportError;
	if (!BuildSecureAreaOperationalReport(
		CurrentCommand,
		ActiveSecureMission,
		ActiveSecureRunId,
		TeamID,
		GetCommandGroupId(),
		SecuredAtSeconds,
		Report,
		ReportError))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Group:%s] Secure Area Report build failed: %s"),
			*GroupID,
			*ReportError.ToString());
		TransitionCurrentCommandStatusForRun(
			ECommandStatus::Failed,
			TEXT("SecureReportBuildFailed"),
			ReportError.ToString(),
			ActiveSecureRunId,
			ExecutionLog);
		return false;
	}

	return SubmitBuiltReportAndComplete(
		Report,
		ActiveSecureRunId,
		ExecutionLog,
		TEXT("AreaSecured"),
		TEXT("SecureReportCreated"),
		TEXT("SecureReportReceived"),
		TEXT("Secure Area"));
}

void AGroupManagerActor::StopSecureMonitoring()
{
	bSecureMonitoringActive = false;
	ActiveSecureMission = FMissionContext();
	ActiveSecureRunId.Invalidate();
	ActiveSecureExecutionLog.Reset();
	ActiveSecureAreaRadius = 0.f;
	SecureExecutionStartedAtSeconds = 0.0;
	SecureControlStartedAtSeconds = -1.0;
	SetActorTickEnabled(false);
}

void AGroupManagerActor::ForceClearCurrentCommand()
{
	CurrentCommand = FCommandIntent();
	bHasCurrentCommand = false;
}

FString AGroupManagerActor::BuildGroupLLMPrompt() const
{
    FString MembersJson;
    for (int32 i = 0; i < Members.Num(); i++)
    {
        if (!IsValid(Members[i])) continue;

        UPersonalityComponent* PC =
            Members[i]->FindComponentByClass<UPersonalityComponent>();
        if (!PC) continue;

        MembersJson += FString::Printf(TEXT(
            "{\"ID\":\"%s\",\"Aggression\":%.2f,\"Fear\":%.2f,"
            "\"Trust\":%.2f,\"Courage\":%.2f,\"Loyalty\":%.2f}%s"
        ), *Members[i]->GetName(), PC->GetAggression(), PC->GetFear(),
           PC->GetTrust(), PC->GetCourage(), PC->GetLoyalty(),
           (i < Members.Num() - 1) ? TEXT(",") : TEXT(""));
    }

    FString MemoryText;
    for (const FGroupMemoryEvent& Mem : GetRecentGroupMemories(8))
    {
        MemoryText += FString::Printf(TEXT("- (목격: %s) %s\n"),
            *Mem.WitnessID, *Mem.Description);
    }

    return FString::Printf(TEXT(
        "You are commanding a small squad in a tactical game.\n"
        "Squad members:\n[%s]\n"
        "Recent events witnessed by the squad:\n%s\n"
        "Respond ONLY in JSON, no markdown. Format:\n"
        "{\"Members\":[{\"ID\":\"name\",\"Aggression\":0.0,\"Fear\":0.0,"
        "\"Trust\":0.0,\"Courage\":0.0,\"Loyalty\":0.0}],"
        "\"Order\":\"HoldFire|FreeFire|Charge|Retreat|TakeCover\"}\n"
        "Each numeric value is a DELTA (-0.3~0.3). "
        "IMPORTANT: identical events can shift different members' personalities "
        "in different directions depending on their existing traits — "
        "do not apply a uniform change to everyone."
    ), *MembersJson, *MemoryText);
}
