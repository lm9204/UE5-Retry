#include "Scenario/ScenarioTypes.h"

#include "Scenario/ScenarioDefinition.h"

#define LOCTEXT_NAMESPACE "ScenarioTypes"

bool TryCreateScenarioRunContext(
	const UScenarioDefinition* Definition,
	const FScenarioLaunchOptions& LaunchOptions,
	FScenarioRunContext& OutContext,
	FText& OutError)
{
	OutContext = FScenarioRunContext();

	if (!IsValid(Definition))
	{
		OutError = LOCTEXT("MissingDefinition", "Scenario Definition이 없습니다.");
		return false;
	}

	if (!Definition->IsDefinitionValid(OutError))
	{
		return false;
	}

	OutContext.RunId = FGuid::NewGuid();
	OutContext.ScenarioId = Definition->ScenarioId;
	OutContext.Level = Definition->Level;
	OutContext.LaunchOptions = LaunchOptions;
	OutContext.bIsActive = true;
	OutError = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
