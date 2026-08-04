#include "UI/ScenarioDebugWidget.h"

#include "Scenario/ScenarioRuntimeSubsystem.h"

#define LOCTEXT_NAMESPACE "ScenarioDebugWidget"

void UScenarioDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshRunContext();
}

void UScenarioDebugWidget::RefreshRunContext()
{
	DisplayedRunContext = FScenarioRunContext();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UScenarioRuntimeSubsystem* RuntimeSubsystem =
			GameInstance->GetSubsystem<UScenarioRuntimeSubsystem>())
		{
			DisplayedRunContext = RuntimeSubsystem->GetCurrentRunContext();
		}
	}

	BP_OnRunContextChanged(DisplayedRunContext);
}

bool UScenarioDebugWidget::RestartCurrentScenario()
{
	UGameInstance* GameInstance = GetGameInstance();
	UScenarioRuntimeSubsystem* RuntimeSubsystem = GameInstance
		? GameInstance->GetSubsystem<UScenarioRuntimeSubsystem>()
		: nullptr;
	if (!RuntimeSubsystem)
	{
		BP_OnScenarioActionFailed(
			LOCTEXT("MissingRuntimeForRestart", "Scenario Runtime Subsystem is unavailable."));
		return false;
	}

	if (!RuntimeSubsystem->RestartCurrentScenario())
	{
		BP_OnScenarioActionFailed(
			LOCTEXT("RestartRejected", "The current scenario could not be restarted. Check the Output Log."));
		return false;
	}

	return true;
}

bool UScenarioDebugWidget::ReturnToScenarioMenu()
{
	UGameInstance* GameInstance = GetGameInstance();
	UScenarioRuntimeSubsystem* RuntimeSubsystem = GameInstance
		? GameInstance->GetSubsystem<UScenarioRuntimeSubsystem>()
		: nullptr;
	if (!RuntimeSubsystem)
	{
		BP_OnScenarioActionFailed(
			LOCTEXT("MissingRuntimeForReturn", "Scenario Runtime Subsystem is unavailable."));
		return false;
	}

	if (!RuntimeSubsystem->ReturnToScenarioMenu())
	{
		BP_OnScenarioActionFailed(
			LOCTEXT("ReturnRejected", "The scenario menu could not be opened. Check the Output Log."));
		return false;
	}

	return true;
}

FScenarioRunContext UScenarioDebugWidget::GetDisplayedRunContext() const
{
	return DisplayedRunContext;
}

#undef LOCTEXT_NAMESPACE
