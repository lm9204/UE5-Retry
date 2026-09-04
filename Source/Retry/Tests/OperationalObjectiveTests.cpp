#include "Misc/AutomationTest.h"

#include "AI/OperationalObjectiveTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace OperationalObjectiveTests
{
	FOperationalFact MakeSecuredFact()
	{
		FOperationalFact Fact;
		Fact.FactId = FGuid::NewGuid();
		Fact.RunId = FGuid::NewGuid();
		Fact.CommandId = FGuid::NewGuid();
		Fact.TeamId = 1;
		Fact.SourceGroupId = TEXT("A");
		Fact.PredicateId = OperationalPredicates::AreaSecured;
		Fact.SubjectId = TEXT("ReconArea_A");
		Fact.Location = FVector(100.f, 200.f, 30.f);
		Fact.ObservedAtSeconds = 10.0;
		return Fact;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOperationalObjectiveBuildsMaintainControl,
	"Retry.Operational.Objective.BuildsMaintainControl",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOperationalObjectiveBuildsMaintainControl::RunTest(
	const FString& Parameters)
{
	const FOperationalFact Fact =
		OperationalObjectiveTests::MakeSecuredFact();
	FOperationalObjective Objective;
	FText Error;
	TestTrue(TEXT("AreaSecured activates a Maintain Control Objective."),
		BuildMaintainAreaControlObjective(
			FGuid::NewGuid(), TEXT("HoldReconArea"), 80,
			Fact, Objective, Error));
	TestTrue(TEXT("The runtime Objective is valid."), Objective.IsValid());
	TestEqual(TEXT("The desired state is explicit."),
		Objective.DesiredStateId,
		OperationalObjectiveStates::MaintainAreaControl);
	TestEqual(TEXT("The trusted Fact supplies the subject."),
		Objective.SubjectId, Fact.SubjectId);
	TestEqual(TEXT("The trusted Fact supplies the position."),
		Objective.TargetLocation, Fact.Location);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOperationalObjectiveRejectsWrongSourceFact,
	"Retry.Operational.Objective.RejectsWrongSourceFact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOperationalObjectiveRejectsWrongSourceFact::RunTest(
	const FString& Parameters)
{
	FOperationalFact Fact = OperationalObjectiveTests::MakeSecuredFact();
	Fact.PredicateId = OperationalPredicates::AreaObserved;
	FOperationalObjective Objective;
	FText Error;
	TestFalse(TEXT("AreaObserved cannot activate Maintain Control."),
		BuildMaintainAreaControlObjective(
			FGuid::NewGuid(), TEXT("HoldReconArea"), 80,
			Fact, Objective, Error));
	TestFalse(TEXT("A rejected build leaves no runtime Objective."),
		Objective.IsValid());
	return true;
}

#endif
