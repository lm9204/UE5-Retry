#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AI/TeamOperationalMemorySubsystem.h"
#include "Scenario/ScenarioFollowUpOrderEvaluator.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ScenarioFollowUpOrderEvaluatorTests
{
	class FScopedTestWorld
	{
	public:
		FScopedTestWorld()
		{
			const FName WorldName = MakeUniqueObjectName(
				nullptr, UWorld::StaticClass(), TEXT("FollowUpEvaluatorWorld"));
			World = NewObject<UWorld>(GetTransientPackage(), WorldName);
			World->WorldType = EWorldType::Game;
			FWorldContext& Context =
				GEngine->CreateNewWorldContext(World->WorldType);
			Context.SetCurrentWorld(World);
			World->AddToRoot();
			World->InitializeNewWorld(UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.CreatePhysicsScene(false)
				.RequiresHitProxies(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false));
		}

		~FScopedTestWorld()
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}

		UTeamOperationalMemorySubsystem* GetMemory() const
		{
			return World->GetSubsystem<UTeamOperationalMemorySubsystem>();
		}

	private:
		UWorld* World = nullptr;
	};

	FScenarioFollowUpOrder MakeOrder()
	{
		FScenarioFollowUpOrder Order;
		Order.Command.CommandId = FGuid::NewGuid();
		Order.Command.AssignedGroupId = TEXT("A");
		FScenarioFactCondition& Condition =
			Order.RequiredFacts.AddDefaulted_GetRef();
		Condition.PredicateId = TEXT("AreaObserved");
		Condition.SubjectId = TEXT("ReconArea_A");
		Condition.SourceGroupId = TEXT("Scout");
		return Order;
	}

	FOperationalReport MakeReport(
		const FGuid& RunId,
		const uint8 TeamId,
		const FName SourceGroupId)
	{
		FOperationalReport Report;
		Report.ReportId = FGuid::NewGuid();
		Report.RunId = RunId;
		Report.CommandId = FGuid::NewGuid();
		Report.TeamId = TeamId;
		Report.SourceGroupId = SourceGroupId;
		FOperationalFact& Fact = Report.Facts.AddDefaulted_GetRef();
		Fact.FactId = FGuid::NewGuid();
		Fact.RunId = RunId;
		Fact.CommandId = Report.CommandId;
		Fact.TeamId = TeamId;
		Fact.SourceGroupId = SourceGroupId;
		Fact.PredicateId = TEXT("AreaObserved");
		Fact.SubjectId = TEXT("ReconArea_A");
		return Report;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioFollowUpWaitsForTeamFact,
	"Retry.Scenario.FollowUpOrders.WaitsForTeamFact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioFollowUpWaitsForTeamFact::RunTest(
	const FString& Parameters)
{
	ScenarioFollowUpOrderEvaluatorTests::FScopedTestWorld TestWorld;
	UTeamOperationalMemorySubsystem* Memory = TestWorld.GetMemory();
	TestNotNull(TEXT("Game World creates Team Memory."), Memory);
	if (!Memory)
	{
		return false;
	}
	const FGuid RunId = FGuid::NewGuid();
	const FScenarioFollowUpOrder Order =
		ScenarioFollowUpOrderEvaluatorTests::MakeOrder();
	TestEqual(TEXT("No received Fact keeps the order waiting."),
		FScenarioFollowUpOrderEvaluator::Evaluate(
			Order, 1, RunId, Memory),
		EScenarioFollowUpReadiness::WaitingForFacts);

	FOperationalReport Received;
	FText Error;
	TestTrue(TEXT("Another team's report is accepted into its own partition."),
		Memory->ReceiveReport(
			ScenarioFollowUpOrderEvaluatorTests::MakeReport(
				RunId, 2, TEXT("Scout")),
			Received, Error));
	TestEqual(TEXT("Another team's Fact cannot release the order."),
		FScenarioFollowUpOrderEvaluator::Evaluate(
			Order, 1, RunId, Memory),
		EScenarioFollowUpReadiness::WaitingForFacts);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioFollowUpPreservesPerGroupOrder,
	"Retry.Scenario.FollowUpOrders.PreservesPerGroupOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioFollowUpPreservesPerGroupOrder::RunTest(
	const FString& Parameters)
{
	FScenarioFollowUpOrder GroupAFirst =
		ScenarioFollowUpOrderEvaluatorTests::MakeOrder();
	FScenarioFollowUpOrder GroupASecond = GroupAFirst;
	FScenarioFollowUpOrder GroupB = GroupAFirst;
	GroupB.Command.AssignedGroupId = TEXT("B");
	const TArray<FScenarioFollowUpOrder> Pending =
		{GroupAFirst, GroupASecond, GroupB};

	TestTrue(TEXT("The first order for Group A may be evaluated."),
		FScenarioFollowUpOrderEvaluator::IsFirstPendingForGroup(
			Pending, 0));
	TestFalse(TEXT("A later Group A order waits for the earlier order."),
		FScenarioFollowUpOrderEvaluator::IsFirstPendingForGroup(
			Pending, 1));
	TestTrue(TEXT("Another group's first order may evaluate independently."),
		FScenarioFollowUpOrderEvaluator::IsFirstPendingForGroup(
			Pending, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioFollowUpRequiresAllFacts,
	"Retry.Scenario.FollowUpOrders.RequiresAllFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioFollowUpRequiresAllFacts::RunTest(
	const FString& Parameters)
{
	ScenarioFollowUpOrderEvaluatorTests::FScopedTestWorld TestWorld;
	UTeamOperationalMemorySubsystem* Memory = TestWorld.GetMemory();
	if (!Memory)
	{
		return false;
	}
	const FGuid RunId = FGuid::NewGuid();
	FScenarioFollowUpOrder Order =
		ScenarioFollowUpOrderEvaluatorTests::MakeOrder();
	FScenarioFactCondition& Second =
		Order.RequiredFacts.AddDefaulted_GetRef();
	Second.PredicateId = TEXT("EnemyKnown");
	Second.SubjectId = TEXT("ReconArea_A");

	FOperationalReport Received;
	FText Error;
	TestTrue(TEXT("The first required report is received."),
		Memory->ReceiveReport(
			ScenarioFollowUpOrderEvaluatorTests::MakeReport(
				RunId, 1, TEXT("Scout")),
			Received, Error));
	TestEqual(TEXT("One missing Fact keeps an all-of gate waiting."),
		FScenarioFollowUpOrderEvaluator::Evaluate(
			Order, 1, RunId, Memory),
		EScenarioFollowUpReadiness::WaitingForFacts);

	Order.RequiredFacts.RemoveAt(1);
	TestEqual(TEXT("All received Facts release the order."),
		FScenarioFollowUpOrderEvaluator::Evaluate(
			Order, 1, RunId, Memory),
		EScenarioFollowUpReadiness::Ready);
	return true;
}

#endif
