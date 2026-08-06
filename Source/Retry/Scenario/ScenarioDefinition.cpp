#include "Scenario/ScenarioDefinition.h"

#include "AI/CommandValidation.h"

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

	TArray<FCommandIntent> ValidatedOpeningOrders;
	if (!BuildOpeningOrders(ValidatedOpeningOrders, OutError))
	{
		return false;
	}

	OutError = FText::GetEmpty();
	return true;
}

bool UScenarioDefinition::BuildOpeningOrders(
	TArray<FCommandIntent>& OutCommands,
	FText& OutError) const
{
	OutCommands.Reset();
	OutCommands.Reserve(OpeningOrders.Num());
	TSet<FName> AssignedGroups;

	for (int32 Index = 0; Index < OpeningOrders.Num(); ++Index)
	{
		FCommandIntent Command = OpeningOrders[Index];
		Command.CommandId = FGuid::NewGuid();
		Command.ParentCommandId.Invalidate();
		Command.Status = ECommandStatus::Proposed;

		const FCommandValidationResult Validation =
			FCommandValidator::Validate(Command);
		if (!Validation.IsValid())
		{
			const FText FirstIssue = Validation.Issues.IsEmpty()
				? LOCTEXT("UnknownOpeningOrderError", "Unknown validation error.")
				: Validation.Issues[0].Message;
			OutError = FText::Format(
				LOCTEXT(
					"InvalidOpeningOrder",
					"Opening Order {0} is invalid: {1}"),
				FText::AsNumber(Index + 1),
				FirstIssue);
			OutCommands.Reset();
			return false;
		}

		if (AssignedGroups.Contains(Command.AssignedGroupId))
		{
			OutError = FText::Format(
				LOCTEXT(
					"DuplicateOpeningOrderGroup",
					"Opening Orders contain more than one active command for group '{0}'."),
				FText::FromName(Command.AssignedGroupId));
			OutCommands.Reset();
			return false;
		}

		AssignedGroups.Add(Command.AssignedGroupId);
		OutCommands.Add(MoveTemp(Command));
	}

	OutError = FText::GetEmpty();
	return true;
}

FPrimaryAssetId UScenarioDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ScenarioDefinition"), ScenarioId);
}

#undef LOCTEXT_NAMESPACE
