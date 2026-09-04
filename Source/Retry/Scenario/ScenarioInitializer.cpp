#include "Scenario/ScenarioInitializer.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "AI/GroupManagerActor.h"
#include "AI/CommanderPlanner.h"
#include "AI/OperationalObjectiveTypes.h"
#include "AI/ScenarioMarkerActor.h"
#include "AI/TeamOperationalMemorySubsystem.h"
#include "Components/MemoryComponent.h"
#include "Components/HealthComponent.h"
#include "LLMRequestQueue.h"
#include "RetryNPCCharacter.h"
#include "Scenario/ScenarioDefinition.h"
#include "Scenario/ScenarioExecutionLogSubsystem.h"
#include "Scenario/ScenarioFollowUpOrderEvaluator.h"
#include "Scenario/ScenarioOperationalObjectiveEvaluator.h"
#include "Scenario/ScenarioRuntimeSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogScenarioInitializer, Log, All);

#define LOCTEXT_NAMESPACE "ScenarioInitializer"

AScenarioInitializer::AScenarioInitializer()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void AScenarioInitializer::BeginPlay()
{
	Super::BeginPlay();

	FText ValidationMessage;
	LastInitializationResult = ValidateSetup(true, ValidationMessage);
	LastValidationMessage = ValidationMessage;

	if (LastInitializationResult == EScenarioInitializationResult::MissingRunContext)
	{
		UE_LOG(LogScenarioInitializer, Warning,
			TEXT("[Scenario] No active run context. Scenario initialization was skipped: %s"),
			*ValidationMessage.ToString());
		return;
	}

	if (LastInitializationResult != EScenarioInitializationResult::Succeeded)
	{
		UE_LOG(LogScenarioInitializer, Error,
			TEXT("[Scenario] Level initialization failed: %s"),
			*ValidationMessage.ToString());
		return;
	}

	const UScenarioRuntimeSubsystem* RuntimeSubsystem =
		GetGameInstance()->GetSubsystem<UScenarioRuntimeSubsystem>();
	const FScenarioRunContext RunContext = RuntimeSubsystem->GetCurrentRunContext();

	FMath::RandInit(RunContext.LaunchOptions.Seed);
	FMath::SRandInit(RunContext.LaunchOptions.Seed);
	ResetRuntimeState();

	UE_LOG(LogScenarioInitializer, Display,
		TEXT("[Scenario] Level initialized. Scenario:%s Run:%s Seed:%d AutoStart:%s"),
		*RunContext.ScenarioId.ToString(),
		*RunContext.RunId.ToString(EGuidFormats::DigitsWithHyphens),
		RunContext.LaunchOptions.Seed,
		RunContext.LaunchOptions.bAutoStart ? TEXT("true") : TEXT("false"));

	if (RunContext.LaunchOptions.bAutoStart)
	{
		GetWorldTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(
				this, &AScenarioInitializer::StartOpeningOrders));
	}
}

void AScenarioInitializer::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	StopFollowUpOrderMonitoring();
	StopCommanderObjectiveMonitoring();
	Super::EndPlay(EndPlayReason);
}

void AScenarioInitializer::ValidateScenarioSetup()
{
	FText ValidationMessage;
	LastInitializationResult = ValidateSetup(false, ValidationMessage);
	LastValidationMessage = ValidationMessage;

	if (LastInitializationResult == EScenarioInitializationResult::Succeeded)
	{
		UE_LOG(LogScenarioInitializer, Display,
			TEXT("[Scenario] Editor validation succeeded: %s"),
			*ValidationMessage.ToString());
	}
	else
	{
		UE_LOG(LogScenarioInitializer, Error,
			TEXT("[Scenario] Editor validation failed: %s"),
			*ValidationMessage.ToString());
	}
}

EScenarioInitializationResult AScenarioInitializer::ValidateSetup(
	const bool bRequireActiveRunContext, FText& OutMessage) const
{
	if (ScenarioDefinition.IsNull())
	{
		OutMessage = LOCTEXT("MissingDefinition", "Scenario Definition is not assigned.");
		return EScenarioInitializationResult::MissingDefinition;
	}

	UScenarioDefinition* Definition = ScenarioDefinition.LoadSynchronous();
	FText DefinitionError;
	if (!IsValid(Definition) || !Definition->IsDefinitionValid(DefinitionError))
	{
		OutMessage = FText::Format(
			LOCTEXT("InvalidDefinition", "Scenario Definition is invalid: {0}"),
			DefinitionError);
		return EScenarioInitializationResult::InvalidDefinition;
	}

	const FString ExpectedLevelName =
		Definition->Level.ToSoftObjectPath().GetAssetName();
	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (!CurrentLevelName.Equals(ExpectedLevelName, ESearchCase::CaseSensitive))
	{
		OutMessage = FText::Format(
			LOCTEXT("LevelMismatch", "Current level '{0}' does not match Definition level '{1}'."),
			FText::FromString(CurrentLevelName), FText::FromString(ExpectedLevelName));
		return EScenarioInitializationResult::LevelMismatch;
	}

	if (!ValidatePlacedActors(OutMessage))
	{
		return EScenarioInitializationResult::InvalidActorConfiguration;
	}

	if (bRequireActiveRunContext)
	{
		const UGameInstance* GameInstance = GetGameInstance();
		const UScenarioRuntimeSubsystem* RuntimeSubsystem = GameInstance
			? GameInstance->GetSubsystem<UScenarioRuntimeSubsystem>()
			: nullptr;
		if (!RuntimeSubsystem || !RuntimeSubsystem->IsScenarioActive())
		{
			OutMessage = LOCTEXT(
				"MissingRunContext",
				"The level was not entered through Scenario Runtime Subsystem.");
			return EScenarioInitializationResult::MissingRunContext;
		}

		const FScenarioRunContext RunContext = RuntimeSubsystem->GetCurrentRunContext();
		if (RunContext.ScenarioId != Definition->ScenarioId)
		{
			OutMessage = FText::Format(
				LOCTEXT("ScenarioMismatch", "Run Context scenario '{0}' does not match '{1}'."),
				FText::FromName(RunContext.ScenarioId),
				FText::FromName(Definition->ScenarioId));
			return EScenarioInitializationResult::ScenarioMismatch;
		}

		if (RunContext.Level.ToSoftObjectPath() != Definition->Level.ToSoftObjectPath())
		{
			OutMessage = LOCTEXT(
				"ContextLevelMismatch",
				"Run Context level does not match the assigned Scenario Definition.");
			return EScenarioInitializationResult::LevelMismatch;
		}
	}

	OutMessage = LOCTEXT("ValidationSucceeded", "Scenario setup is valid.");
	return EScenarioInitializationResult::Succeeded;
}

bool AScenarioInitializer::ValidatePlacedActors(FText& OutMessage) const
{
	TArray<AActor*> InitializerActors;
	UGameplayStatics::GetAllActorsOfClass(this, StaticClass(), InitializerActors);

	TArray<AActor*> GroupActors;
	UGameplayStatics::GetAllActorsOfClass(this, AGroupManagerActor::StaticClass(), GroupActors);

	TArray<AActor*> NPCActors;
	UGameplayStatics::GetAllActorsOfClass(this, ARetryNPCCharacter::StaticClass(), NPCActors);

	TArray<AActor*> MarkerActors;
	UGameplayStatics::GetAllActorsOfClass(
		this, AScenarioMarkerActor::StaticClass(), MarkerActors);

	TArray<FString> Errors;
	TSet<FString> GroupIds;
	TSet<AGroupManagerActor*> KnownGroups;
	TMap<AGroupManagerActor*, int32> MemberCounts;
	TMap<AGroupManagerActor*, int32> LeaderCounts;

	if (InitializerActors.Num() != 1)
	{
		Errors.Add(FString::Printf(
			TEXT("The level must contain exactly one Scenario Initializer (current: %d)."),
			InitializerActors.Num()));
	}

	if (GroupActors.IsEmpty())
	{
		Errors.Add(TEXT("No Group Manager Actor is placed in the level."));
	}

	for (AActor* Actor : GroupActors)
	{
		AGroupManagerActor* Group = CastChecked<AGroupManagerActor>(Actor);
		const FString TrimmedGroupId = Group->GroupID.TrimStartAndEnd();
		KnownGroups.Add(Group);
		MemberCounts.Add(Group, 0);
		LeaderCounts.Add(Group, 0);

		if (TrimmedGroupId.IsEmpty())
		{
			Errors.Add(FString::Printf(TEXT("Group '%s' has an empty Group ID."), *Group->GetName()));
		}
		else if (GroupIds.Contains(TrimmedGroupId))
		{
			Errors.Add(FString::Printf(TEXT("Group ID '%s' is duplicated."), *TrimmedGroupId));
		}
		else
		{
			GroupIds.Add(TrimmedGroupId);
		}
	}

	if (NPCActors.IsEmpty())
	{
		Errors.Add(TEXT("No Retry NPC Character is placed in the level."));
	}

	TSet<FString> NPCNames;
	for (AActor* Actor : NPCActors)
	{
		ARetryNPCCharacter* NPC = CastChecked<ARetryNPCCharacter>(Actor);
		const FString TrimmedName = NPC->NPCName.TrimStartAndEnd();
		if (TrimmedName.IsEmpty())
		{
			Errors.Add(FString::Printf(TEXT("NPC '%s' has an empty NPC Name."), *NPC->GetName()));
		}
		else if (NPCNames.Contains(TrimmedName))
		{
			Errors.Add(FString::Printf(TEXT("NPC Name '%s' is duplicated."), *TrimmedName));
		}
		else
		{
			NPCNames.Add(TrimmedName);
		}

		if (!IsValid(NPC->MyGroup) || !KnownGroups.Contains(NPC->MyGroup))
		{
			Errors.Add(FString::Printf(
				TEXT("NPC '%s' does not reference a Group Manager in this level."),
				*NPC->GetName()));
			continue;
		}

		++MemberCounts.FindChecked(NPC->MyGroup);
		if (NPC->bIsGroupLeader)
		{
			++LeaderCounts.FindChecked(NPC->MyGroup);
		}

		if (NPC->TeamID != NPC->MyGroup->TeamID)
		{
			Errors.Add(FString::Printf(
				TEXT("NPC '%s' Team ID %u does not match Group '%s' Team ID %u."),
				*NPC->GetName(), NPC->TeamID, *NPC->MyGroup->GroupID,
				NPC->MyGroup->TeamID));
		}
	}

	for (AActor* Actor : GroupActors)
	{
		AGroupManagerActor* Group = CastChecked<AGroupManagerActor>(Actor);
		const int32 MemberCount = MemberCounts.FindChecked(Group);
		const int32 LeaderCount = LeaderCounts.FindChecked(Group);
		if (MemberCount == 0)
		{
			Errors.Add(FString::Printf(TEXT("Group '%s' has no assigned NPC."), *Group->GroupID));
		}
		if (LeaderCount != 1)
		{
			Errors.Add(FString::Printf(
				TEXT("Group '%s' must have exactly one leader (current: %d)."),
				*Group->GroupID, LeaderCount));
		}
	}

	if (!MarkerActors.IsEmpty())
	{
		TArray<AScenarioMarkerActor*> Markers;
		Markers.Reserve(MarkerActors.Num());
		for (AActor* Actor : MarkerActors)
		{
			Markers.Add(CastChecked<AScenarioMarkerActor>(Actor));
		}

		const FScenarioMarkerValidationResult MarkerValidation =
			AScenarioMarkerActor::ValidateMarkerSet(Markers);
		for (const FScenarioMarkerValidationIssue& Issue
			: MarkerValidation.Issues)
		{
			Errors.Add(Issue.Message.ToString());
		}
	}

	if (Errors.IsEmpty())
	{
		return true;
	}

	OutMessage = FText::FromString(FString::Join(Errors, TEXT("\n")));
	return false;
}

void AScenarioInitializer::ResetRuntimeState() const
{
	if (UWorld* World = GetWorld())
	{
		if (UTeamOperationalMemorySubsystem* TeamMemory =
			World->GetSubsystem<UTeamOperationalMemorySubsystem>())
		{
			TeamMemory->ResetOperationalMemory();
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULLMRequestQueue* LLMQueue = GameInstance->GetSubsystem<ULLMRequestQueue>())
		{
			LLMQueue->ResetQueueForScenarioTransition();
		}
	}

	TArray<AActor*> GroupActors;
	UGameplayStatics::GetAllActorsOfClass(this, AGroupManagerActor::StaticClass(), GroupActors);
	for (AActor* Actor : GroupActors)
	{
		CastChecked<AGroupManagerActor>(Actor)->ResetGroupRuntimeState();
	}

	TArray<AActor*> NPCActors;
	UGameplayStatics::GetAllActorsOfClass(this, ARetryNPCCharacter::StaticClass(), NPCActors);
	for (AActor* Actor : NPCActors)
	{
		if (UMemoryComponent* Memory = CastChecked<ARetryNPCCharacter>(Actor)->MemoryComponent)
		{
			Memory->ResetMemories();
		}
	}
}

void AScenarioInitializer::StartOpeningOrders()
{
	UGameInstance* GameInstance = GetGameInstance();
	UScenarioRuntimeSubsystem* Runtime = GameInstance
		? GameInstance->GetSubsystem<UScenarioRuntimeSubsystem>()
		: nullptr;
	if (!Runtime)
	{
		return;
	}

	const FScenarioRunContext RunContext = Runtime->GetCurrentRunContext();
	UScenarioDefinition* Definition = ScenarioDefinition.LoadSynchronous();
	if (!RunContext.IsValid()
		|| !IsValid(Definition)
		|| RunContext.ScenarioId != Definition->ScenarioId)
	{
		UE_LOG(LogScenarioInitializer, Warning,
			TEXT("[Scenario] Opening Orders skipped because the active Run changed."));
		return;
	}

	TArray<FCommandIntent> Commands;
	TArray<FScenarioFollowUpOrder> FollowUpOrders;
	TArray<FScenarioOperationalObjective> OperationalObjectives;
	FText BuildError;
	if (!Definition->BuildOpeningOrders(Commands, BuildError)
		|| !Definition->BuildFollowUpOrders(FollowUpOrders, BuildError)
		|| !Definition->BuildOperationalObjectives(
			OperationalObjectives, BuildError))
	{
		UE_LOG(LogScenarioInitializer, Error,
			TEXT("[Scenario] Scenario Orders are invalid: %s"),
			*BuildError.ToString());
		return;
	}

	if (Commands.IsEmpty()
		&& FollowUpOrders.IsEmpty()
		&& OperationalObjectives.IsEmpty())
	{
		UE_LOG(LogScenarioInitializer, Display,
			TEXT("[Scenario] Auto Start has no Opening Orders."));
		return;
	}

	TArray<AActor*> GroupActors;
	UGameplayStatics::GetAllActorsOfClass(
		this, AGroupManagerActor::StaticClass(), GroupActors);
	TMap<FName, AGroupManagerActor*> GroupsById;
	for (AActor* Actor : GroupActors)
	{
		AGroupManagerActor* Group = CastChecked<AGroupManagerActor>(Actor);
		GroupsById.Add(FName(*Group->GroupID.TrimStartAndEnd()), Group);
	}

	UScenarioExecutionLogSubsystem* ExecutionLog =
		GameInstance->GetSubsystem<UScenarioExecutionLogSubsystem>();
	for (const FCommandIntent& Command : Commands)
	{
		AGroupManagerActor* const* GroupPtr =
			GroupsById.Find(Command.AssignedGroupId);
		if (!GroupPtr || !IsValid(*GroupPtr))
		{
			UE_LOG(LogScenarioInitializer, Error,
				TEXT("[Scenario] Opening Order group '%s' was not found."),
				*Command.AssignedGroupId.ToString());
			continue;
		}

		StartScenarioCommand(
			Command, *GroupPtr, RunContext.RunId,
			ExecutionLog, TEXT("Opening Order"));
	}

	BeginFollowUpOrderMonitoring(
		MoveTemp(FollowUpOrders), RunContext.RunId);
	BeginCommanderObjectiveMonitoring(
		MoveTemp(OperationalObjectives), RunContext.RunId);
}

bool AScenarioInitializer::StartScenarioCommand(
	const FCommandIntent& Command,
	AGroupManagerActor* Group,
	const FGuid& RunId,
	UScenarioExecutionLogSubsystem* ExecutionLog,
	const TCHAR* OrderLabel)
{
	if (!IsValid(Group) || !ExecutionLog || !RunId.IsValid())
	{
		return false;
	}

	const FCommandAssignmentResult Assignment =
		Group->AssignCommandForRun(Command, RunId, ExecutionLog);
	if (!Assignment.IsSuccess())
	{
		UE_LOG(LogScenarioInitializer, Error,
			TEXT("[Scenario] %s assignment failed for group '%s': %s"),
			OrderLabel,
			*Command.AssignedGroupId.ToString(),
			*Assignment.Message.ToString());
		return false;
	}

	const FGroupMissionDispatchResult Dispatch =
		Group->DispatchCurrentMissionForRun(RunId, ExecutionLog);
	if (!Dispatch.IsSuccess())
	{
		UE_LOG(LogScenarioInitializer, Error,
			TEXT("[Scenario] %s dispatch failed for group '%s': %s"),
			OrderLabel,
			*Command.AssignedGroupId.ToString(),
			*Dispatch.Message.ToString());
		Group->CancelCurrentCommand(TEXT("ScenarioCommandDispatchFailed"));
		Group->ClearCurrentCommand();
		return false;
	}

	UE_LOG(LogScenarioInitializer, Display,
		TEXT("[Scenario] %s executing. Group:%s Command:%s Recipients:%d"),
		OrderLabel,
		*Command.AssignedGroupId.ToString(),
		*Command.CommandId.ToString(EGuidFormats::DigitsWithHyphens),
		Dispatch.RecipientCount);
	return true;
}

void AScenarioInitializer::BeginFollowUpOrderMonitoring(
	TArray<FScenarioFollowUpOrder> Orders,
	const FGuid& RunId)
{
	StopFollowUpOrderMonitoring();
	if (Orders.IsEmpty() || !RunId.IsValid() || !GetWorld())
	{
		return;
	}

	PendingFollowUpOrders = MoveTemp(Orders);
	FollowUpRunId = RunId;
	GetWorldTimerManager().SetTimer(
		FollowUpEvaluationTimer,
		this,
		&AScenarioInitializer::EvaluateFollowUpOrders,
		0.2f,
		true,
		0.2f);
	UE_LOG(LogScenarioInitializer, Display,
		TEXT("[Scenario] Follow Up monitoring started. Run:%s Pending:%d"),
		*RunId.ToString(EGuidFormats::DigitsWithHyphens),
		PendingFollowUpOrders.Num());
}

void AScenarioInitializer::EvaluateFollowUpOrders()
{
	UGameInstance* GameInstance = GetGameInstance();
	UScenarioRuntimeSubsystem* Runtime = GameInstance
		? GameInstance->GetSubsystem<UScenarioRuntimeSubsystem>()
		: nullptr;
	const FScenarioRunContext RunContext = Runtime
		? Runtime->GetCurrentRunContext()
		: FScenarioRunContext();
	if (!RunContext.IsValid() || RunContext.RunId != FollowUpRunId)
	{
		StopFollowUpOrderMonitoring();
		return;
	}

	UTeamOperationalMemorySubsystem* TeamMemory = GetWorld()
		? GetWorld()->GetSubsystem<UTeamOperationalMemorySubsystem>()
		: nullptr;
	UScenarioExecutionLogSubsystem* ExecutionLog = GameInstance
		? GameInstance->GetSubsystem<UScenarioExecutionLogSubsystem>()
		: nullptr;
	if (!TeamMemory || !ExecutionLog)
	{
		return;
	}

	TArray<AActor*> GroupActors;
	UGameplayStatics::GetAllActorsOfClass(
		this, AGroupManagerActor::StaticClass(), GroupActors);
	TMap<FName, AGroupManagerActor*> GroupsById;
	for (AActor* Actor : GroupActors)
	{
		AGroupManagerActor* Group = CastChecked<AGroupManagerActor>(Actor);
		GroupsById.Add(FName(*Group->GroupID.TrimStartAndEnd()), Group);
	}

	for (int32 Index = 0; Index < PendingFollowUpOrders.Num(); ++Index)
	{
		const FName GroupId =
			PendingFollowUpOrders[Index].Command.AssignedGroupId;
		if (!FScenarioFollowUpOrderEvaluator::IsFirstPendingForGroup(
			PendingFollowUpOrders, Index))
		{
			continue;
		}

		AGroupManagerActor* const* GroupPtr = GroupsById.Find(GroupId);
		if (!GroupPtr || !IsValid(*GroupPtr))
		{
			UE_LOG(LogScenarioInitializer, Error,
				TEXT("[Scenario] Follow Up Order group '%s' was not found."),
				*GroupId.ToString());
			PendingFollowUpOrders.RemoveAt(Index--);
			continue;
		}

		AGroupManagerActor* Group = *GroupPtr;
		const EScenarioFollowUpReadiness Readiness =
			FScenarioFollowUpOrderEvaluator::Evaluate(
				PendingFollowUpOrders[Index], Group->TeamID,
				FollowUpRunId, TeamMemory);
		if (Readiness == EScenarioFollowUpReadiness::WaitingForFacts)
		{
			continue;
		}
		if (Readiness == EScenarioFollowUpReadiness::InvalidInput)
		{
			UE_LOG(LogScenarioInitializer, Error,
				TEXT("[Scenario] Follow Up Order for group '%s' became invalid."),
				*GroupId.ToString());
			PendingFollowUpOrders.RemoveAt(Index--);
			continue;
		}

		if (Group->HasCurrentCommand())
		{
			if (!IsCommandStatusTerminal(Group->GetCurrentCommand().Status)
				|| !Group->ClearCurrentCommand())
			{
				continue;
			}
		}

		const FCommandIntent Command =
			PendingFollowUpOrders[Index].Command;
		StartScenarioCommand(
			Command, Group, FollowUpRunId,
			ExecutionLog, TEXT("Follow Up Order"));
		PendingFollowUpOrders.RemoveAt(Index--);
	}

	if (PendingFollowUpOrders.IsEmpty())
	{
		StopFollowUpOrderMonitoring();
	}
}

void AScenarioInitializer::StopFollowUpOrderMonitoring()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(FollowUpEvaluationTimer);
	}
	PendingFollowUpOrders.Reset();
	FollowUpRunId.Invalidate();
}

void AScenarioInitializer::BeginCommanderObjectiveMonitoring(
	TArray<FScenarioOperationalObjective> Objectives,
	const FGuid& RunId)
{
	StopCommanderObjectiveMonitoring();
	if (Objectives.IsEmpty() || !RunId.IsValid() || !GetWorld())
	{
		return;
	}

	PendingOperationalObjectives = MoveTemp(Objectives);
	CommanderRunId = RunId;
	GetWorldTimerManager().SetTimer(
		CommanderEvaluationTimer,
		this,
		&AScenarioInitializer::EvaluateCommanderObjectives,
		0.5f,
		true,
		0.5f);
	UE_LOG(LogScenarioInitializer, Display,
		TEXT("[Scenario] Commander objective monitoring started. Run:%s Pending:%d"),
		*RunId.ToString(EGuidFormats::DigitsWithHyphens),
		PendingOperationalObjectives.Num());
}

void AScenarioInitializer::EvaluateCommanderObjectives()
{
	UGameInstance* GameInstance = GetGameInstance();
	UScenarioRuntimeSubsystem* Runtime = GameInstance
		? GameInstance->GetSubsystem<UScenarioRuntimeSubsystem>()
		: nullptr;
	const FScenarioRunContext RunContext = Runtime
		? Runtime->GetCurrentRunContext()
		: FScenarioRunContext();
	if (!RunContext.IsValid() || RunContext.RunId != CommanderRunId)
	{
		StopCommanderObjectiveMonitoring();
		return;
	}

	UTeamOperationalMemorySubsystem* TeamMemory = GetWorld()
		? GetWorld()->GetSubsystem<UTeamOperationalMemorySubsystem>()
		: nullptr;
	UScenarioExecutionLogSubsystem* ExecutionLog = GameInstance
		? GameInstance->GetSubsystem<UScenarioExecutionLogSubsystem>()
		: nullptr;
	if (!TeamMemory || !ExecutionLog)
	{
		return;
	}

	TArray<AActor*> GroupActors;
	UGameplayStatics::GetAllActorsOfClass(
		this, AGroupManagerActor::StaticClass(), GroupActors);
	TMap<FName, AGroupManagerActor*> GroupsById;
	TArray<FCommanderGroupState> GroupStates;
	for (AActor* Actor : GroupActors)
	{
		AGroupManagerActor* Group = CastChecked<AGroupManagerActor>(Actor);
		const FName GroupId(*Group->GroupID.TrimStartAndEnd());
		GroupsById.Add(GroupId, Group);

		FCommanderGroupState& State = GroupStates.AddDefaulted_GetRef();
		State.GroupId = GroupId;
		State.TeamId = Group->TeamID;
		const bool bCommandAvailable = !Group->HasCurrentCommand()
			|| IsCommandStatusTerminal(Group->GetCurrentCommand().Status);
		State.bIsAvailable = !GroupId.IsNone()
			&& IsValid(Group->Leader)
			&& Group->Leader->HealthComponent
			&& !Group->Leader->HealthComponent->IsDead()
			&& bCommandAvailable;
	}

	for (int32 Index = 0;
		Index < PendingOperationalObjectives.Num();
		++Index)
	{
		const FScenarioOperationalObjective& Definition =
			PendingOperationalObjectives[Index];
		FOperationalFact AreaSecuredFact;
		const EScenarioOperationalObjectiveReadiness Readiness =
			FScenarioOperationalObjectiveEvaluator::Evaluate(
				Definition,
				CommanderRunId,
				TeamMemory->GetFactsForTeam(Definition.TeamId),
				AreaSecuredFact);
		if (Readiness
			== EScenarioOperationalObjectiveReadiness::WaitingForFacts)
		{
			continue;
		}
		if (Readiness
			== EScenarioOperationalObjectiveReadiness::InvalidInput)
		{
			UE_LOG(LogScenarioInitializer, Error,
				TEXT("[Scenario] Operational Objective '%s' became invalid."),
				*Definition.ObjectiveId.ToString());
			PendingOperationalObjectives.RemoveAt(Index--);
			continue;
		}

		FOperationalObjective Objective;
		FText ObjectiveError;
		if (!BuildMaintainAreaControlObjective(
			FGuid::NewGuid(),
			Definition.ObjectiveId,
			Definition.Priority,
			AreaSecuredFact,
			Objective,
			ObjectiveError))
		{
			UE_LOG(LogScenarioInitializer, Error,
				TEXT("[Scenario] Operational Objective '%s' activation failed: %s"),
				*Definition.ObjectiveId.ToString(),
				*ObjectiveError.ToString());
			PendingOperationalObjectives.RemoveAt(Index--);
			continue;
		}

		const FCommanderPlanningResult Plan = FCommanderPlanner::Plan(
			Objective, GroupStates, FGuid::NewGuid(), TEXT("HQ"));
		if (Plan.Outcome == ECommanderPlanningOutcome::NoAvailableGroup)
		{
			continue;
		}
		if (!Plan.IsSuccess())
		{
			UE_LOG(LogScenarioInitializer, Error,
				TEXT("[Scenario] Commander could not plan Objective '%s'."),
				*Definition.ObjectiveId.ToString());
			PendingOperationalObjectives.RemoveAt(Index--);
			continue;
		}

		AGroupManagerActor* const* GroupPtr =
			GroupsById.Find(Plan.Command.AssignedGroupId);
		AGroupManagerActor* Group = GroupPtr ? *GroupPtr : nullptr;
		if (!IsValid(Group))
		{
			PendingOperationalObjectives.RemoveAt(Index--);
			continue;
		}
		if (Group->HasCurrentCommand()
			&& !Group->ClearCurrentCommand())
		{
			continue;
		}

		const bool bStarted = StartScenarioCommand(
			Plan.Command,
			Group,
			CommanderRunId,
			ExecutionLog,
			TEXT("Commander Order"));
		if (bStarted)
		{
			UE_LOG(LogScenarioInitializer, Display,
				TEXT("[Scenario] Commander Objective planned. Objective:%s Group:%s Command:%s"),
				*Objective.ObjectiveId.ToString(),
				*Plan.Command.AssignedGroupId.ToString(),
				*Plan.Command.CommandId.ToString(EGuidFormats::DigitsWithHyphens));
		}
		else
		{
			UE_LOG(LogScenarioInitializer, Error,
				TEXT("[Scenario] Commander Objective failed. Objective:%s Group:%s Command:%s"),
				*Objective.ObjectiveId.ToString(),
				*Plan.Command.AssignedGroupId.ToString(),
				*Plan.Command.CommandId.ToString(EGuidFormats::DigitsWithHyphens));
			continue;
		}
		PendingOperationalObjectives.RemoveAt(Index--);
	}

	if (PendingOperationalObjectives.IsEmpty())
	{
		StopCommanderObjectiveMonitoring();
	}
}

void AScenarioInitializer::StopCommanderObjectiveMonitoring()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(CommanderEvaluationTimer);
	}
	PendingOperationalObjectives.Reset();
	CommanderRunId.Invalidate();
}

#undef LOCTEXT_NAMESPACE
