#include "Scenario/ScenarioRegistrySettings.h"

#include "Scenario/ScenarioDefinition.h"

#define LOCTEXT_NAMESPACE "ScenarioRegistrySettings"

bool UScenarioRegistrySettings::IsRegistryValid(TArray<FText>& OutErrors) const
{
	OutErrors.Reset();

	TSet<FName> SeenScenarioIds;
	for (int32 Index = 0; Index < RegisteredScenarios.Num(); ++Index)
	{
		const TSoftObjectPtr<UScenarioDefinition>& ScenarioReference = RegisteredScenarios[Index];
		if (ScenarioReference.IsNull())
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("NullScenarioReference", "등록 목록의 {0}번 Scenario가 비어 있습니다."),
				FText::AsNumber(Index)));
			continue;
		}

		UScenarioDefinition* Scenario = ScenarioReference.LoadSynchronous();
		if (!IsValid(Scenario))
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("ScenarioLoadFailed", "등록 목록의 {0}번 Scenario를 로드하지 못했습니다: {1}"),
				FText::AsNumber(Index),
				FText::FromString(ScenarioReference.ToSoftObjectPath().ToString())));
			continue;
		}

		FText DefinitionError;
		if (!Scenario->IsDefinitionValid(DefinitionError))
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("InvalidScenarioDefinition", "Scenario '{0}' 설정이 잘못되었습니다: {1}"),
				FText::FromName(Scenario->ScenarioId),
				DefinitionError));
			continue;
		}

		if (SeenScenarioIds.Contains(Scenario->ScenarioId))
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("DuplicateScenarioId", "Scenario ID '{0}'가 중복 등록되었습니다."),
				FText::FromName(Scenario->ScenarioId)));
			continue;
		}

		SeenScenarioIds.Add(Scenario->ScenarioId);
	}

	return OutErrors.IsEmpty();
}

FName UScenarioRegistrySettings::GetCategoryName() const
{
	return TEXT("Game");
}

#undef LOCTEXT_NAMESPACE
