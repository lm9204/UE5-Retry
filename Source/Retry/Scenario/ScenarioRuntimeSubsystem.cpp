#include "Scenario/ScenarioRuntimeSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

#include "LLMRequestQueue.h"
#include "Scenario/ScenarioDefinition.h"
#include "Scenario/ScenarioExecutionLogSubsystem.h"
#include "Scenario/ScenarioRegistrySettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogScenarioRuntime, Log, All);

namespace ScenarioRuntime
{
	const FString ScenarioMenuPackageName = TEXT("/Game/Scenarios/Maps/Lvl_ScenarioMenu");
}

void UScenarioRuntimeSubsystem::Deinitialize()
{
	CurrentRunContext = FScenarioRunContext();
	Super::Deinitialize();
}

TArray<UScenarioDefinition*> UScenarioRuntimeSubsystem::GetRegisteredScenarios() const
{
	TArray<UScenarioDefinition*> Scenarios;
	LoadValidatedScenarios(Scenarios);
	return Scenarios;
}

bool UScenarioRuntimeSubsystem::StartScenario(
	const FName ScenarioId, const FScenarioLaunchOptions& LaunchOptions)
{
	return StartScenarioInternal(
		ScenarioId, LaunchOptions, EScenarioRunEndReason::Replaced);
}

bool UScenarioRuntimeSubsystem::StartScenarioInternal(
	const FName ScenarioId,
	const FScenarioLaunchOptions& LaunchOptions,
	const EScenarioRunEndReason PreviousRunEndReason)
{
	if (!GetWorld())
	{
		UE_LOG(LogScenarioRuntime, Error,
			TEXT("[Scenario] World가 없어 Scenario '%s'를 시작할 수 없습니다."),
			*ScenarioId.ToString());
		return false;
	}

	UScenarioDefinition* Definition = FindRegisteredScenario(ScenarioId);
	if (!Definition)
	{
		UE_LOG(LogScenarioRuntime, Error,
			TEXT("[Scenario] 등록된 Scenario '%s'를 찾지 못했습니다."),
			*ScenarioId.ToString());
		return false;
	}

	FScenarioRunContext NewContext;
	FText Error;
	if (!TryCreateScenarioRunContext(Definition, LaunchOptions, NewContext, Error))
	{
		UE_LOG(LogScenarioRuntime, Error,
			TEXT("[Scenario] Scenario '%s' 시작 준비 실패: %s"),
			*ScenarioId.ToString(), *Error.ToString());
		return false;
	}

	CurrentRunContext = NewContext;
	ResetLLMQueueForTransition();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UScenarioExecutionLogSubsystem* ExecutionLog =
			GameInstance->GetSubsystem<UScenarioExecutionLogSubsystem>())
		{
			ExecutionLog->StartRun(CurrentRunContext, PreviousRunEndReason);
		}
	}

	UE_LOG(LogScenarioRuntime, Display,
		TEXT("[Scenario] 시작 — Scenario:%s, Run:%s, Seed:%d"),
		*CurrentRunContext.ScenarioId.ToString(),
		*CurrentRunContext.RunId.ToString(EGuidFormats::DigitsWithHyphens),
		CurrentRunContext.LaunchOptions.Seed);

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, CurrentRunContext.Level);
	return true;
}

bool UScenarioRuntimeSubsystem::RestartCurrentScenario()
{
	if (!CurrentRunContext.IsValid())
	{
		UE_LOG(LogScenarioRuntime, Warning,
			TEXT("[Scenario] 활성 Scenario가 없어 Restart를 거부했습니다."));
		return false;
	}

	const FName ScenarioId = CurrentRunContext.ScenarioId;
	const FScenarioLaunchOptions LaunchOptions = CurrentRunContext.LaunchOptions;
	return StartScenarioInternal(
		ScenarioId, LaunchOptions, EScenarioRunEndReason::Restarted);
}

bool UScenarioRuntimeSubsystem::ReturnToScenarioMenu()
{
	if (!GetWorld())
	{
		UE_LOG(LogScenarioRuntime, Error,
			TEXT("[Scenario] World가 없어 Scenario 메뉴로 돌아갈 수 없습니다."));
		return false;
	}

	if (!FPackageName::DoesPackageExist(ScenarioRuntime::ScenarioMenuPackageName))
	{
		UE_LOG(LogScenarioRuntime, Error,
			TEXT("[Scenario] 메뉴 Level이 아직 없습니다: %s"),
			*ScenarioRuntime::ScenarioMenuPackageName);
		return false;
	}

	ResetLLMQueueForTransition();
	const FGuid PreviousRunId = CurrentRunContext.RunId;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UScenarioExecutionLogSubsystem* ExecutionLog =
			GameInstance->GetSubsystem<UScenarioExecutionLogSubsystem>())
		{
			ExecutionLog->EndActiveRun(
				EScenarioRunEndReason::ReturnedToMenu);
		}
	}
	CurrentRunContext = FScenarioRunContext();

	UE_LOG(LogScenarioRuntime, Display,
		TEXT("[Scenario] Returning to menu. Previous Run:%s"),
		*PreviousRunId.ToString(EGuidFormats::DigitsWithHyphens));

	UGameplayStatics::OpenLevel(
		this, FName(*ScenarioRuntime::ScenarioMenuPackageName));
	return true;
}

FScenarioRunContext UScenarioRuntimeSubsystem::GetCurrentRunContext() const
{
	return CurrentRunContext;
}

bool UScenarioRuntimeSubsystem::IsScenarioActive() const
{
	return CurrentRunContext.IsValid();
}

bool UScenarioRuntimeSubsystem::LoadValidatedScenarios(
	TArray<UScenarioDefinition*>& OutScenarios) const
{
	OutScenarios.Reset();

	const UScenarioRegistrySettings* Registry = GetDefault<UScenarioRegistrySettings>();
	if (!Registry)
	{
		UE_LOG(LogScenarioRuntime, Error, TEXT("[Scenario] Registry Settings가 없습니다."));
		return false;
	}

	TArray<FText> Errors;
	if (!Registry->IsRegistryValid(Errors))
	{
		for (const FText& Error : Errors)
		{
			UE_LOG(LogScenarioRuntime, Error,
				TEXT("[Scenario] Registry 오류: %s"), *Error.ToString());
		}
		return false;
	}

	for (const TSoftObjectPtr<UScenarioDefinition>& ScenarioReference
		: Registry->RegisteredScenarios)
	{
		if (UScenarioDefinition* Scenario = ScenarioReference.LoadSynchronous())
		{
			OutScenarios.Add(Scenario);
		}
	}

	return true;
}

UScenarioDefinition* UScenarioRuntimeSubsystem::FindRegisteredScenario(
	const FName ScenarioId) const
{
	TArray<UScenarioDefinition*> Scenarios;
	if (!LoadValidatedScenarios(Scenarios))
	{
		return nullptr;
	}

	for (UScenarioDefinition* Scenario : Scenarios)
	{
		if (IsValid(Scenario) && Scenario->ScenarioId == ScenarioId)
		{
			return Scenario;
		}
	}

	return nullptr;
}

void UScenarioRuntimeSubsystem::ResetLLMQueueForTransition() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULLMRequestQueue* LLMQueue = GameInstance->GetSubsystem<ULLMRequestQueue>())
		{
			LLMQueue->ResetQueueForScenarioTransition();
		}
	}
}
