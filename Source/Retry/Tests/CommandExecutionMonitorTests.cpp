#include "Misc/AutomationTest.h"

#include "AI/CommandExecutionMonitor.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FReconExecutionRangeUsesNavigationShape,
	"Retry.Command.ExecutionMonitor.UsesHorizontalAndVerticalArrivalTolerances",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReconExecutionRangeUsesNavigationShape::RunTest(
	const FString& Parameters)
{
	const FVector MarkerLocation(0.f, 0.f, 0.f);
	TestTrue(TEXT("Capsule-center height does not reject a horizontally arrived NPC."),
		FCommandExecutionMonitor::IsWithinObservationRange(
			FVector(140.f, 0.f, 90.f), MarkerLocation, 150.f, 240.f));
	TestFalse(TEXT("A different floor is not treated as arrival."),
		FCommandExecutionMonitor::IsWithinObservationRange(
			FVector(0.f, 0.f, 300.f), MarkerLocation, 150.f, 240.f));
	TestFalse(TEXT("Horizontal distance still respects the arrival radius."),
		FCommandExecutionMonitor::IsWithinObservationRange(
			FVector(151.f, 0.f, 0.f), MarkerLocation, 150.f, 240.f));
	return true;
}

namespace CommandExecutionMonitorTests
{
	FCommandIntent MakeCommand()
	{
		FCommandIntent Command;
		Command.CommandId = FGuid::NewGuid();
		Command.Verb = ECommandVerb::Recon;
		Command.TargetType = ECommandTargetType::Area;
		Command.Status = ECommandStatus::Executing;
		return Command;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommandExecutionMonitorRequiresArrivalAndHold,
	"Retry.Command.ExecutionMonitor.RequiresArrivalAndHold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandExecutionMonitorRequiresArrivalAndHold::RunTest(
	const FString& Parameters)
{
	FCommandIntent Command = CommandExecutionMonitorTests::MakeCommand();
	Command.CompletionCriteria.MinimumHoldSeconds = 3.f;
	FReconExecutionSnapshot Snapshot;
	Snapshot.bLeaderAvailable = true;
	Snapshot.bAtObservationPoint = true;
	Snapshot.bObservationAllowed = true;
	Snapshot.ExecutionElapsedSeconds = 5.0;
	Snapshot.StableObservationSeconds = 2.9;
	TestEqual(TEXT("Early observation keeps holding."),
		FCommandExecutionMonitor::EvaluateRecon(Command, Snapshot).Outcome,
		EReconExecutionOutcome::WaitingForObservationHold);
	Snapshot.StableObservationSeconds = 3.0;
	TestEqual(TEXT("Required hold makes observation ready."),
		FCommandExecutionMonitor::EvaluateRecon(Command, Snapshot).Outcome,
		EReconExecutionOutcome::ObservationReady);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommandExecutionMonitorWaitsDuringCombat,
	"Retry.Command.ExecutionMonitor.WaitsDuringCombat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandExecutionMonitorWaitsDuringCombat::RunTest(
	const FString& Parameters)
{
	const FCommandIntent Command =
		CommandExecutionMonitorTests::MakeCommand();
	FReconExecutionSnapshot Snapshot;
	Snapshot.bLeaderAvailable = true;
	Snapshot.bAtObservationPoint = true;
	Snapshot.bObservationAllowed = false;
	Snapshot.ExecutionElapsedSeconds = 5.0;
	TestEqual(TEXT("Combat prevents observation completion."),
		FCommandExecutionMonitor::EvaluateRecon(Command, Snapshot).Outcome,
		EReconExecutionOutcome::WaitingForArrival);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommandExecutionMonitorReportsFailures,
	"Retry.Command.ExecutionMonitor.ReportsFailures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandExecutionMonitorReportsFailures::RunTest(
	const FString& Parameters)
{
	FCommandIntent Command = CommandExecutionMonitorTests::MakeCommand();
	Command.CompletionCriteria.TimeoutSeconds = 10.f;
	FReconExecutionSnapshot Snapshot;
	Snapshot.bLeaderAvailable = false;
	Snapshot.ExecutionElapsedSeconds = 1.0;
	TestEqual(TEXT("Missing leader has a distinct failure."),
		FCommandExecutionMonitor::EvaluateRecon(Command, Snapshot).Outcome,
		EReconExecutionOutcome::FailedLeaderUnavailable);
	Snapshot.bLeaderAvailable = true;
	Snapshot.ExecutionElapsedSeconds = 10.0;
	TestEqual(TEXT("Timeout has a distinct failure."),
		FCommandExecutionMonitor::EvaluateRecon(Command, Snapshot).Outcome,
		EReconExecutionOutcome::FailedTimeout);
	return true;
}

#endif
