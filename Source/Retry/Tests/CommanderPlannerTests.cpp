#include "Misc/AutomationTest.h"

#include "AI/CommandValidation.h"
#include "AI/CommanderPlanner.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace CommanderPlannerTests
{
	FOperationalObjective MakeObjective()
	{
		FOperationalObjective Objective;
		Objective.ObjectiveInstanceId = FGuid::NewGuid();
		Objective.RunId = FGuid::NewGuid();
		Objective.SourceFactId = FGuid::NewGuid();
		Objective.ObjectiveId = TEXT("HoldReconArea");
		Objective.DesiredStateId =
			OperationalObjectiveStates::MaintainAreaControl;
		Objective.SubjectId = TEXT("ReconArea_A");
		Objective.TargetLocation = FVector(100.f, 200.f, 30.f);
		Objective.TeamId = 1;
		Objective.Priority = 80;
		return Objective;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderPlannerBuildsDefendCommand,
	"Retry.Commander.Planner.BuildsDefendCommand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderPlannerBuildsDefendCommand::RunTest(
	const FString& Parameters)
{
	const FGuid CommandId = FGuid::NewGuid();
	const TArray<FCommanderGroupState> Groups =
	{
		{TEXT("B"), 2, true},
		{TEXT("A2"), 1, true},
		{TEXT("A1"), 1, true},
	};
	const FCommanderPlanningResult Result = FCommanderPlanner::Plan(
		CommanderPlannerTests::MakeObjective(),
		Groups, CommandId, TEXT("HQ"));

	TestTrue(TEXT("Maintain Control produces a Command."),
		Result.IsSuccess());
	TestEqual(TEXT("The deterministic first friendly group is selected."),
		Result.Command.AssignedGroupId, FName(TEXT("A1")));
	TestEqual(TEXT("Maintain Control maps to Defend."),
		Result.Command.Verb, ECommandVerb::Defend);
	TestEqual(TEXT("The trusted location becomes a Position target."),
		Result.Command.TargetType, ECommandTargetType::Position);
	TestEqual(TEXT("The caller-owned Command identity is preserved."),
		Result.Command.CommandId, CommandId);
	TestTrue(TEXT("The planned Structured Command passes validation."),
		FCommandValidator::Validate(Result.Command).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderPlannerWaitsForAvailableFriendlyGroup,
	"Retry.Commander.Planner.RequiresAvailableFriendlyGroup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderPlannerWaitsForAvailableFriendlyGroup::RunTest(
	const FString& Parameters)
{
	const TArray<FCommanderGroupState> Groups =
	{
		{TEXT("A"), 1, false},
		{TEXT("B"), 2, true},
	};
	const FCommanderPlanningResult Result = FCommanderPlanner::Plan(
		CommanderPlannerTests::MakeObjective(),
		Groups, FGuid::NewGuid(), TEXT("HQ"));
	TestEqual(TEXT("Busy allies and available enemies are not candidates."),
		Result.Outcome, ECommanderPlanningOutcome::NoAvailableGroup);
	return true;
}

#endif
