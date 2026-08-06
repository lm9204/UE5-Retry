#include "Misc/AutomationTest.h"

#include "AI/AreaControlEvaluator.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace AreaControlEvaluatorTests
{
	FCommandIntent MakeCommand()
	{
		FCommandIntent Command;
		Command.CommandId = FGuid::NewGuid();
		Command.Verb = ECommandVerb::Secure;
		Command.TargetType = ECommandTargetType::Area;
		Command.Status = ECommandStatus::Executing;
		return Command;
	}

	FAreaControlSnapshot MakeViableSnapshot()
	{
		FAreaControlSnapshot Snapshot;
		Snapshot.bLeaderAvailable = true;
		Snapshot.bLeaderInsideArea = true;
		Snapshot.LivingGroupMemberCount = 2;
		Snapshot.FriendlyCountInside = 2;
		return Snapshot;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAreaControlRequiresEntryAndClearArea,
	"Retry.Mission.AreaControl.RequiresEntryAndClearArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAreaControlRequiresEntryAndClearArea::RunTest(
	const FString& Parameters)
{
	const FCommandIntent Command = AreaControlEvaluatorTests::MakeCommand();
	FAreaControlSnapshot Snapshot =
		AreaControlEvaluatorTests::MakeViableSnapshot();
	Snapshot.bLeaderInsideArea = false;
	TestEqual(TEXT("The leader must enter the Area."),
		FAreaControlEvaluator::EvaluateSecureArea(Command, Snapshot).Outcome,
		EAreaControlOutcome::WaitingForEntry);
	Snapshot.bLeaderInsideArea = true;
	Snapshot.HostileCountInside = 1;
	TestEqual(TEXT("A living hostile contests control."),
		FAreaControlEvaluator::EvaluateSecureArea(Command, Snapshot).Outcome,
		EAreaControlOutcome::Contested);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAreaControlRequiresStableHold,
	"Retry.Mission.AreaControl.RequiresStableHold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAreaControlRequiresStableHold::RunTest(
	const FString& Parameters)
{
	FCommandIntent Command = AreaControlEvaluatorTests::MakeCommand();
	Command.CompletionCriteria.MinimumHoldSeconds = 5.f;
	FAreaControlSnapshot Snapshot =
		AreaControlEvaluatorTests::MakeViableSnapshot();
	Snapshot.StableControlSeconds = 4.9;
	TestEqual(TEXT("Control remains pending before the hold completes."),
		FAreaControlEvaluator::EvaluateSecureArea(Command, Snapshot).Outcome,
		EAreaControlOutcome::HoldingControl);
	Snapshot.StableControlSeconds = 5.0;
	TestEqual(TEXT("Stable uncontested control secures the Area."),
		FAreaControlEvaluator::EvaluateSecureArea(Command, Snapshot).Outcome,
		EAreaControlOutcome::Secured);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAreaControlReportsDistinctFailures,
	"Retry.Mission.AreaControl.ReportsDistinctFailures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAreaControlReportsDistinctFailures::RunTest(
	const FString& Parameters)
{
	FCommandIntent Command = AreaControlEvaluatorTests::MakeCommand();
	Command.CompletionCriteria.TimeoutSeconds = 10.f;
	FAreaControlSnapshot Snapshot =
		AreaControlEvaluatorTests::MakeViableSnapshot();
	Snapshot.bLeaderAvailable = false;
	TestEqual(TEXT("Leader loss has a distinct outcome."),
		FAreaControlEvaluator::EvaluateSecureArea(Command, Snapshot).Outcome,
		EAreaControlOutcome::FailedLeaderUnavailable);
	Snapshot.bLeaderAvailable = true;
	Snapshot.LivingGroupMemberCount = 0;
	TestEqual(TEXT("No combat power has a distinct outcome."),
		FAreaControlEvaluator::EvaluateSecureArea(Command, Snapshot).Outcome,
		EAreaControlOutcome::FailedNoCombatPower);
	Snapshot.LivingGroupMemberCount = 1;
	Snapshot.ExecutionElapsedSeconds = 10.0;
	TestEqual(TEXT("Timeout has a distinct outcome."),
		FAreaControlEvaluator::EvaluateSecureArea(Command, Snapshot).Outcome,
		EAreaControlOutcome::FailedTimeout);
	Command.CompletionCriteria.MinimumHoldSeconds = 11.f;
	TestEqual(TEXT("An impossible hold/timeout pair is invalid."),
		FAreaControlEvaluator::EvaluateSecureArea(Command, Snapshot).Outcome,
		EAreaControlOutcome::InvalidCommand);
	return true;
}

#endif
