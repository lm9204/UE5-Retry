#include "Scenario/ScenarioInitializer.h"

#include "Kismet/GameplayStatics.h"

#include "AI/GroupManagerActor.h"
#include "AI/ScenarioMarkerActor.h"
#include "Components/MemoryComponent.h"
#include "LLMRequestQueue.h"
#include "RetryNPCCharacter.h"
#include "Scenario/ScenarioDefinition.h"
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

#undef LOCTEXT_NAMESPACE
