#include "GroupManagerActor.h"

#include "AIController.h"
#include "Engine/GameInstance.h"
#include "LLMRequestQueue.h"
#include "RetryNPCCharacter.h"
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
}

AGroupManagerActor::AGroupManagerActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // 렌더링/충돌 없는 순수 데이터·로직 액터
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
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
        UE_LOG(LogTemp, Warning,
            TEXT("[Group:%s] 임계값 초과 — LLM 요청"), *GroupID);

        UGameInstance* GI = GetGameInstance();
        if (GI)
        {
            if (ULLMRequestQueue* Queue = GI->GetSubsystem<ULLMRequestQueue>())
            {
                Queue->EnqueueGroupRequest(this);  // 아래 추가
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

    Leader = nullptr;

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
	return TryTransitionCommandStatus(
		CurrentCommand, NewStatus, TransitionError);
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
	ForceClearCurrentCommand();

    // Leader and Members describe the placed level configuration. They may already
    // have been registered depending on actor BeginPlay order, so preserve them.
    SetOrderForAll(ENPCOrder::HoldFire, 0.f);
}

FName AGroupManagerActor::GetCommandGroupId() const
{
	return FName(*GroupID.TrimStartAndEnd());
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
