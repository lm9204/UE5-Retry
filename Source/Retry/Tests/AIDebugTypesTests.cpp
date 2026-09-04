#include "Misc/AutomationTest.h"

#include "Debug/AIDebugTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace AIDebugTypesTests
{
	FCommandIntent MakeCommand()
	{
		FCommandIntent Command;
		Command.CommandId = FGuid::NewGuid();
		Command.Verb = ECommandVerb::Defend;
		Command.Status = ECommandStatus::Executing;
		return Command;
	}

	FMissionContext MakeMission(const FGuid& CommandId)
	{
		FMissionContext Mission;
		Mission.CommandId = CommandId;
		Mission.ObjectiveId = TEXT("ReconArea_A");
		Mission.ObjectiveLocation = FVector(100.f, 200.f, 30.f);
		return Mission;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAIDebugSnapshotReportsSynchronizedMission,
	"Retry.Debug.AI.Snapshot.ReportsSynchronizedMission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIDebugSnapshotReportsSynchronizedMission::RunTest(
	const FString& Parameters)
{
	const FCommandIntent Command = AIDebugTypesTests::MakeCommand();
	const FMissionContext Mission =
		AIDebugTypesTests::MakeMission(Command.CommandId);
	const FAIMissionDebugSnapshot Snapshot = BuildAIMissionDebugSnapshot(
		true, Mission, true, &Command,
		true, Mission.ObjectiveLocation, true);

	TestTrue(TEXT("The current Command is visible."), Snapshot.bHasCommand);
	TestTrue(TEXT("The active Mission is visible."), Snapshot.bHasMission);
	TestTrue(TEXT("Command and Mission identity match."),
		Snapshot.bCommandMatchesMission);
	TestTrue(TEXT("Mission and Blackboard projection match."),
		Snapshot.bBlackboardSynchronized);
	TestEqual(TEXT("The Objective ID is preserved."),
		Snapshot.ObjectiveId, Mission.ObjectiveId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAIDebugSnapshotDistinguishesCombatSuspension,
	"Retry.Debug.AI.Snapshot.DistinguishesCombatSuspension",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIDebugSnapshotDistinguishesCombatSuspension::RunTest(
	const FString& Parameters)
{
	const FCommandIntent Command = AIDebugTypesTests::MakeCommand();
	const FMissionContext Mission =
		AIDebugTypesTests::MakeMission(Command.CommandId);
	const FAIMissionDebugSnapshot Snapshot = BuildAIMissionDebugSnapshot(
		true, Mission, false, &Command,
		true, Mission.ObjectiveLocation, false);

	TestTrue(TEXT("The Mission remains active during combat."),
		Snapshot.bHasMission);
	TestFalse(TEXT("Mission movement is suspended during combat."),
		Snapshot.bMissionMovementAllowed);
	TestTrue(TEXT("A synchronized false gate is not a data error."),
		Snapshot.bBlackboardSynchronized);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAIDebugSnapshotDetectsProjectionMismatch,
	"Retry.Debug.AI.Snapshot.DetectsProjectionMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIDebugSnapshotDetectsProjectionMismatch::RunTest(
	const FString& Parameters)
{
	const FCommandIntent Command = AIDebugTypesTests::MakeCommand();
	const FMissionContext Mission =
		AIDebugTypesTests::MakeMission(Command.CommandId);
	const FAIMissionDebugSnapshot Snapshot = BuildAIMissionDebugSnapshot(
		true, Mission, true, &Command,
		true, FVector::ZeroVector, false);

	TestFalse(TEXT("Wrong target and gate are reported as a mismatch."),
		Snapshot.bBlackboardSynchronized);
	return true;
}

#endif
