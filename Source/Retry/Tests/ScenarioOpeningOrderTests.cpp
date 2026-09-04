#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "Scenario/ScenarioDefinition.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ScenarioOpeningOrderTests
{
	UScenarioDefinition* MakeDefinition()
	{
		UScenarioDefinition* Definition = NewObject<UScenarioDefinition>();
		Definition->ScenarioId = TEXT("OpeningOrderTest");
		Definition->DisplayName = FText::FromString(TEXT("Opening Order Test"));
		Definition->Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(
			TEXT("/Game/Tests/DummyScenarioLevel.DummyScenarioLevel")));
		return Definition;
	}

	FCommandIntent MakeOpeningOrder(const FName GroupId = TEXT("A"))
	{
		FCommandIntent Order;
		Order.IssuerId = TEXT("HQ");
		Order.AssignedGroupId = GroupId;
		Order.Verb = ECommandVerb::Recon;
		Order.TargetType = ECommandTargetType::Area;
		Order.TargetId = TEXT("ReconArea_A");
		return Order;
	}

	FScenarioFollowUpOrder MakeFollowUpOrder(
		const FName GroupId = TEXT("A"))
	{
		FScenarioFollowUpOrder FollowUp;
		FollowUp.Command = MakeOpeningOrder(GroupId);
		FollowUp.Command.Verb = ECommandVerb::Secure;
		FScenarioFactCondition& Condition =
			FollowUp.RequiredFacts.AddDefaulted_GetRef();
		Condition.PredicateId = TEXT("AreaObserved");
		Condition.SubjectId = TEXT("ReconArea_A");
		return FollowUp;
	}

	FScenarioOperationalObjective MakeOperationalObjective()
	{
		FScenarioOperationalObjective Objective;
		Objective.ObjectiveId = TEXT("HoldReconArea");
		Objective.DesiredStateId =
			OperationalObjectiveStates::MaintainAreaControl;
		Objective.SubjectId = TEXT("ReconArea_A");
		Objective.TeamId = 1;
		Objective.Priority = 80;
		FScenarioFactCondition& Condition =
			Objective.ActivationFacts.AddDefaulted_GetRef();
		Condition.PredicateId = OperationalPredicates::AreaSecured;
		Condition.SubjectId = TEXT("ReconArea_A");
		return Objective;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioOperationalObjectivesValidateSchema,
	"Retry.Scenario.OperationalObjectives.ValidatesSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioOperationalObjectivesValidateSchema::RunTest(
	const FString& Parameters)
{
	UScenarioDefinition* Definition =
		ScenarioOpeningOrderTests::MakeDefinition();
	Definition->OperationalObjectives.Add(
		ScenarioOpeningOrderTests::MakeOperationalObjective());

	TArray<FScenarioOperationalObjective> Objectives;
	FText Error;
	TestTrue(TEXT("A supported Objective with AreaSecured activation builds."),
		Definition->BuildOperationalObjectives(Objectives, Error));
	TestEqual(TEXT("The authored Objective is preserved."),
		Objectives.Num(), 1);

	FScenarioOperationalObjective Invalid =
		ScenarioOpeningOrderTests::MakeOperationalObjective();
	Invalid.ActivationFacts[0].PredicateId =
		OperationalPredicates::AreaObserved;
	Definition->OperationalObjectives.Reset();
	Definition->OperationalObjectives.Add(Invalid);
	TestFalse(TEXT("Maintain Control requires AreaSecured activation."),
		Definition->BuildOperationalObjectives(Objectives, Error));
	TestFalse(TEXT("Definition validation includes Operational Objectives."),
		Definition->IsDefinitionValid(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioFollowUpOrdersCreateRuntimeIdentity,
	"Retry.Scenario.FollowUpOrders.CreatesRuntimeIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioFollowUpOrdersCreateRuntimeIdentity::RunTest(
	const FString& Parameters)
{
	UScenarioDefinition* Definition =
		ScenarioOpeningOrderTests::MakeDefinition();
	Definition->FollowUpOrders.Add(
		ScenarioOpeningOrderTests::MakeFollowUpOrder());

	TArray<FScenarioFollowUpOrder> FirstBuild;
	TArray<FScenarioFollowUpOrder> SecondBuild;
	FText Error;
	TestTrue(TEXT("Valid Follow Up Order builds."),
		Definition->BuildFollowUpOrders(FirstBuild, Error));
	TestTrue(TEXT("The same Follow Up template builds for another Run."),
		Definition->BuildFollowUpOrders(SecondBuild, Error));
	if (FirstBuild.Num() != 1 || SecondBuild.Num() != 1)
	{
		return false;
	}
	TestTrue(TEXT("Follow Up Command receives a runtime identity."),
		FirstBuild[0].Command.CommandId.IsValid());
	TestTrue(TEXT("Each Run receives a new Follow Up identity."),
		FirstBuild[0].Command.CommandId
			!= SecondBuild[0].Command.CommandId);
	TestEqual(TEXT("The authored Fact condition is preserved."),
		FirstBuild[0].RequiredFacts[0].PredicateId,
		FName(TEXT("AreaObserved")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioFollowUpOrdersRejectInvalidConditions,
	"Retry.Scenario.FollowUpOrders.RejectsInvalidConditions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioFollowUpOrdersRejectInvalidConditions::RunTest(
	const FString& Parameters)
{
	UScenarioDefinition* Definition =
		ScenarioOpeningOrderTests::MakeDefinition();
	FScenarioFollowUpOrder MissingCondition =
		ScenarioOpeningOrderTests::MakeFollowUpOrder();
	MissingCondition.RequiredFacts.Reset();
	Definition->FollowUpOrders.Add(MissingCondition);

	TArray<FScenarioFollowUpOrder> Orders;
	FText Error;
	TestFalse(TEXT("A Follow Up Order without a gate is rejected."),
		Definition->BuildFollowUpOrders(Orders, Error));
	TestTrue(TEXT("Rejected build remains empty."), Orders.IsEmpty());

	Definition->FollowUpOrders.Reset();
	FScenarioFollowUpOrder DuplicateCondition =
		ScenarioOpeningOrderTests::MakeFollowUpOrder();
	const FScenarioFactCondition RepeatedCondition =
		DuplicateCondition.RequiredFacts[0];
	DuplicateCondition.RequiredFacts.Add(RepeatedCondition);
	Definition->FollowUpOrders.Add(DuplicateCondition);
	TestFalse(TEXT("Duplicate Fact gates are rejected."),
		Definition->BuildFollowUpOrders(Orders, Error));
	TestTrue(TEXT("Definition validation includes Follow Up Orders."),
		!Definition->IsDefinitionValid(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioOpeningOrdersCreateRuntimeIdentity,
	"Retry.Scenario.OpeningOrders.CreatesRuntimeIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioOpeningOrdersCreateRuntimeIdentity::RunTest(
	const FString& Parameters)
{
	UScenarioDefinition* Definition =
		ScenarioOpeningOrderTests::MakeDefinition();
	Definition->OpeningOrders.Add(
		ScenarioOpeningOrderTests::MakeOpeningOrder());

	TArray<FCommandIntent> FirstBuild;
	TArray<FCommandIntent> SecondBuild;
	FText Error;
	TestTrue(TEXT("Valid Opening Order builds."),
		Definition->BuildOpeningOrders(FirstBuild, Error));
	TestTrue(TEXT("The same template can build another Run."),
		Definition->BuildOpeningOrders(SecondBuild, Error));
	TestEqual(TEXT("One template produces one Command."),
		FirstBuild.Num(), 1);
	if (FirstBuild.Num() != 1 || SecondBuild.Num() != 1)
	{
		return false;
	}
	TestTrue(TEXT("Runtime Command receives a valid identity."),
		FirstBuild[0].CommandId.IsValid());
	TestTrue(TEXT("Each Run receives a new Command identity."),
		FirstBuild[0].CommandId != SecondBuild[0].CommandId);
	TestEqual(TEXT("Authoring group is preserved."),
		FirstBuild[0].AssignedGroupId, FName(TEXT("A")));
	TestEqual(TEXT("Runtime Command starts as Proposed."),
		FirstBuild[0].Status, ECommandStatus::Proposed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioOpeningOrdersRejectInvalidTemplate,
	"Retry.Scenario.OpeningOrders.RejectsInvalidTemplate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioOpeningOrdersRejectInvalidTemplate::RunTest(
	const FString& Parameters)
{
	UScenarioDefinition* Definition =
		ScenarioOpeningOrderTests::MakeDefinition();
	FCommandIntent InvalidOrder =
		ScenarioOpeningOrderTests::MakeOpeningOrder();
	InvalidOrder.TargetId = NAME_None;
	Definition->OpeningOrders.Add(InvalidOrder);

	TArray<FCommandIntent> Commands;
	FText Error;
	TestFalse(TEXT("Invalid Opening Order is rejected."),
		Definition->BuildOpeningOrders(Commands, Error));
	TestTrue(TEXT("Rejected build has no partial Commands."),
		Commands.IsEmpty());
	TestTrue(TEXT("Definition validation includes Opening Orders."),
		!Definition->IsDefinitionValid(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioOpeningOrdersRejectDuplicateGroup,
	"Retry.Scenario.OpeningOrders.RejectsDuplicateGroup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioOpeningOrdersRejectDuplicateGroup::RunTest(
	const FString& Parameters)
{
	UScenarioDefinition* Definition =
		ScenarioOpeningOrderTests::MakeDefinition();
	Definition->OpeningOrders.Add(
		ScenarioOpeningOrderTests::MakeOpeningOrder(TEXT("A")));
	FCommandIntent SecondOrder =
		ScenarioOpeningOrderTests::MakeOpeningOrder(TEXT("A"));
	SecondOrder.TargetId = TEXT("ReconArea_B");
	Definition->OpeningOrders.Add(SecondOrder);

	TArray<FCommandIntent> Commands;
	FText Error;
	TestFalse(TEXT("One group cannot receive two Opening Orders."),
		Definition->BuildOpeningOrders(Commands, Error));
	TestTrue(TEXT("Duplicate group rejection leaves no Commands."),
		Commands.IsEmpty());
	return true;
}

#endif
