#include "Misc/AutomationTest.h"

#include "Scenario/ScenarioOperationalObjectiveEvaluator.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ScenarioOperationalObjectiveEvaluatorTests
{
	FScenarioOperationalObjective MakeDefinition()
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
		Condition.SourceGroupId = TEXT("A");
		return Objective;
	}

	FOperationalFact MakeFact(const FGuid& RunId, const uint8 TeamId)
	{
		FOperationalFact Fact;
		Fact.FactId = FGuid::NewGuid();
		Fact.RunId = RunId;
		Fact.CommandId = FGuid::NewGuid();
		Fact.TeamId = TeamId;
		Fact.SourceGroupId = TEXT("A");
		Fact.PredicateId = OperationalPredicates::AreaSecured;
		Fact.SubjectId = TEXT("ReconArea_A");
		Fact.Location = FVector(50.f, 60.f, 70.f);
		Fact.ObservedAtSeconds = 2.0;
		return Fact;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioObjectiveWaitsForMatchingFact,
	"Retry.Scenario.OperationalObjectives.WaitsForMatchingFact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioObjectiveWaitsForMatchingFact::RunTest(
	const FString& Parameters)
{
	const FGuid RunId = FGuid::NewGuid();
	FOperationalFact ActivatedBy;
	TestEqual(TEXT("No Fact keeps the Objective pending."),
		FScenarioOperationalObjectiveEvaluator::Evaluate(
			ScenarioOperationalObjectiveEvaluatorTests::MakeDefinition(),
			RunId, {}, ActivatedBy),
		EScenarioOperationalObjectiveReadiness::WaitingForFacts);
	TestEqual(TEXT("Another team's Fact cannot activate the Objective."),
		FScenarioOperationalObjectiveEvaluator::Evaluate(
			ScenarioOperationalObjectiveEvaluatorTests::MakeDefinition(),
			RunId,
			{ScenarioOperationalObjectiveEvaluatorTests::MakeFact(RunId, 2)},
			ActivatedBy),
		EScenarioOperationalObjectiveReadiness::WaitingForFacts);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioObjectiveActivatesFromAreaSecured,
	"Retry.Scenario.OperationalObjectives.ActivatesFromAreaSecured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioObjectiveActivatesFromAreaSecured::RunTest(
	const FString& Parameters)
{
	const FGuid RunId = FGuid::NewGuid();
	const FOperationalFact Fact =
		ScenarioOperationalObjectiveEvaluatorTests::MakeFact(RunId, 1);
	FOperationalFact ActivatedBy;
	TestEqual(TEXT("The matching received Fact activates the Objective."),
		FScenarioOperationalObjectiveEvaluator::Evaluate(
			ScenarioOperationalObjectiveEvaluatorTests::MakeDefinition(),
			RunId, {Fact}, ActivatedBy),
		EScenarioOperationalObjectiveReadiness::Ready);
	TestEqual(TEXT("Activation preserves the trusted source Fact."),
		ActivatedBy.FactId, Fact.FactId);
	return true;
}

#endif
