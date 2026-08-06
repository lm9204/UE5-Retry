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
