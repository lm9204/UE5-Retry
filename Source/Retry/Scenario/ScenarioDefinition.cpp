#include "Scenario/ScenarioDefinition.h"

#include "AI/CommandValidation.h"

#define LOCTEXT_NAMESPACE "ScenarioDefinition"

namespace ScenarioDefinition
{
	bool BuildRuntimeCommand(
		const FCommandIntent& Template,
		const int32 DisplayIndex,
		const FText& OrderLabel,
		FCommandIntent& OutCommand,
		FText& OutError)
	{
		OutCommand = Template;
		OutCommand.CommandId = FGuid::NewGuid();
		OutCommand.ParentCommandId.Invalidate();
		OutCommand.Status = ECommandStatus::Proposed;

		const FCommandValidationResult Validation =
			FCommandValidator::Validate(OutCommand);
		if (Validation.IsValid())
		{
			return true;
		}

		const FText FirstIssue = Validation.Issues.IsEmpty()
			? LOCTEXT("UnknownScenarioOrderError", "Unknown validation error.")
			: Validation.Issues[0].Message;
		OutError = FText::Format(
			LOCTEXT(
				"InvalidScenarioOrder",
				"{0} {1} is invalid: {2}"),
			OrderLabel,
			FText::AsNumber(DisplayIndex),
			FirstIssue);
		return false;
	}
}

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
	TArray<FScenarioFollowUpOrder> ValidatedFollowUpOrders;
	if (!BuildFollowUpOrders(ValidatedFollowUpOrders, OutError))
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
		FCommandIntent Command;
		if (!ScenarioDefinition::BuildRuntimeCommand(
			OpeningOrders[Index], Index + 1,
			LOCTEXT("OpeningOrderLabel", "Opening Order"),
			Command, OutError))
		{
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

bool UScenarioDefinition::BuildFollowUpOrders(
	TArray<FScenarioFollowUpOrder>& OutOrders,
	FText& OutError) const
{
	OutOrders.Reset();
	OutOrders.Reserve(FollowUpOrders.Num());

	for (int32 Index = 0; Index < FollowUpOrders.Num(); ++Index)
	{
		FScenarioFollowUpOrder Order = FollowUpOrders[Index];
		if (Order.RequiredFacts.IsEmpty())
		{
			OutError = FText::Format(
				LOCTEXT(
					"MissingFollowUpConditions",
					"Follow Up Order {0} requires at least one Fact condition."),
				FText::AsNumber(Index + 1));
			OutOrders.Reset();
			return false;
		}

		TSet<FString> UniqueConditions;
		for (const FScenarioFactCondition& Condition : Order.RequiredFacts)
		{
			if (Condition.PredicateId.IsNone() || Condition.SubjectId.IsNone())
			{
				OutError = FText::Format(
					LOCTEXT(
						"InvalidFollowUpCondition",
						"Follow Up Order {0} has a Fact condition without a predicate or subject."),
					FText::AsNumber(Index + 1));
				OutOrders.Reset();
				return false;
			}

			const FString ConditionKey = FString::Printf(
				TEXT("%s|%s|%s"),
				*Condition.PredicateId.ToString(),
				*Condition.SubjectId.ToString(),
				*Condition.SourceGroupId.ToString());
			if (UniqueConditions.Contains(ConditionKey))
			{
				OutError = FText::Format(
					LOCTEXT(
						"DuplicateFollowUpCondition",
						"Follow Up Order {0} contains a duplicate Fact condition."),
					FText::AsNumber(Index + 1));
				OutOrders.Reset();
				return false;
			}
			UniqueConditions.Add(ConditionKey);
		}

		FCommandIntent RuntimeCommand;
		if (!ScenarioDefinition::BuildRuntimeCommand(
			Order.Command, Index + 1,
			LOCTEXT("FollowUpOrderLabel", "Follow Up Order"),
			RuntimeCommand, OutError))
		{
			OutOrders.Reset();
			return false;
		}
		Order.Command = MoveTemp(RuntimeCommand);

		OutOrders.Add(MoveTemp(Order));
	}

	OutError = FText::GetEmpty();
	return true;
}

FPrimaryAssetId UScenarioDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ScenarioDefinition"), ScenarioId);
}

#undef LOCTEXT_NAMESPACE
