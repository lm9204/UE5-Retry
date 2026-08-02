#include "Scenario/ScenarioDefinition.h"

#define LOCTEXT_NAMESPACE "ScenarioDefinition"

bool UScenarioDefinition::IsDefinitionValid(FText& OutError) const
{
	if (ScenarioId.IsNone())
	{
		OutError = LOCTEXT("MissingScenarioId", "Scenario ID가 비어 있습니다.");
		return false;
	}

	if (DisplayName.IsEmpty())
	{
		OutError = LOCTEXT("MissingDisplayName", "Display Name이 비어 있습니다.");
		return false;
	}

	if (Level.IsNull())
	{
		OutError = LOCTEXT("MissingLevel", "실행할 Level이 지정되지 않았습니다.");
		return false;
	}

	OutError = FText::GetEmpty();
	return true;
}

FPrimaryAssetId UScenarioDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ScenarioDefinition"), ScenarioId);
}

#undef LOCTEXT_NAMESPACE
