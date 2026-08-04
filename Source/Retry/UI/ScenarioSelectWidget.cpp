#include "UI/ScenarioSelectWidget.h"

#include "Scenario/ScenarioDefinition.h"
#include "Scenario/ScenarioRuntimeSubsystem.h"

#define LOCTEXT_NAMESPACE "ScenarioSelectWidget"

void UScenarioSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshScenarioList();
}

void UScenarioSelectWidget::RefreshScenarioList()
{
	AvailableScenarios.Reset();

	UGameInstance* GameInstance = GetGameInstance();
	UScenarioRuntimeSubsystem* RuntimeSubsystem = GameInstance
		? GameInstance->GetSubsystem<UScenarioRuntimeSubsystem>()
		: nullptr;

	if (RuntimeSubsystem)
	{
		for (UScenarioDefinition* Scenario : RuntimeSubsystem->GetRegisteredScenarios())
		{
			if (IsValid(Scenario))
			{
				AvailableScenarios.Add(Scenario);
			}
		}
	}

	if (!AvailableScenarios.Contains(SelectedScenario))
	{
		SelectedScenario = AvailableScenarios.IsEmpty()
			? nullptr
			: AvailableScenarios[0];
		SelectedLaunchOptions = SelectedScenario
			? SelectedScenario->DefaultLaunchOptions
			: FScenarioLaunchOptions();
	}

	BP_RefreshScenarioList(GetAvailableScenarios());
	BP_OnScenarioSelectionChanged(SelectedScenario, SelectedLaunchOptions);
}

bool UScenarioSelectWidget::SelectScenario(UScenarioDefinition* Scenario)
{
	if (!IsValid(Scenario) || !AvailableScenarios.Contains(Scenario))
	{
		return false;
	}

	SelectedScenario = Scenario;
	SelectedLaunchOptions = Scenario->DefaultLaunchOptions;
	BP_OnScenarioSelectionChanged(SelectedScenario, SelectedLaunchOptions);
	return true;
}

void UScenarioSelectWidget::SetSelectedLaunchOptions(
	const FScenarioLaunchOptions& LaunchOptions)
{
	SelectedLaunchOptions = LaunchOptions;
}

bool UScenarioSelectWidget::StartSelectedScenario()
{
	if (!IsValid(SelectedScenario))
	{
		BP_OnScenarioStartFailed(
			LOCTEXT("NoSelectedScenario", "Select a scenario before starting."));
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UScenarioRuntimeSubsystem* RuntimeSubsystem = GameInstance
		? GameInstance->GetSubsystem<UScenarioRuntimeSubsystem>()
		: nullptr;
	if (!RuntimeSubsystem)
	{
		BP_OnScenarioStartFailed(
			LOCTEXT("MissingRuntimeSubsystem", "Scenario Runtime Subsystem is unavailable."));
		return false;
	}

	if (!RuntimeSubsystem->StartScenario(
		SelectedScenario->ScenarioId, SelectedLaunchOptions))
	{
		BP_OnScenarioStartFailed(
			LOCTEXT("StartRejected", "The selected scenario could not be started. Check the Output Log."));
		return false;
	}

	return true;
}

TArray<UScenarioDefinition*> UScenarioSelectWidget::GetAvailableScenarios() const
{
	TArray<UScenarioDefinition*> Result;
	Result.Reserve(AvailableScenarios.Num());
	for (UScenarioDefinition* Scenario : AvailableScenarios)
	{
		Result.Add(Scenario);
	}
	return Result;
}

UScenarioDefinition* UScenarioSelectWidget::GetSelectedScenario() const
{
	return SelectedScenario;
}

FScenarioLaunchOptions UScenarioSelectWidget::GetSelectedLaunchOptions() const
{
	return SelectedLaunchOptions;
}

#undef LOCTEXT_NAMESPACE
