#include "Misc/AutomationTest.h"

#include "Components/NPCDecisionComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace MissionOverlayTests
{
	FMissionContext MakeValidMission()
	{
		FMissionContext Mission;
		Mission.CommandId = FGuid::NewGuid();
		Mission.ObjectiveId = TEXT("ReconObs_A1");
		Mission.ObjectiveLocation = FVector(100.f, 200.f, 300.f);
		return Mission;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMissionOverlayAcceptsAndClearsValidContext,
	"Retry.Mission.Overlay.AcceptsAndClearsValidContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMissionOverlayAcceptsAndClearsValidContext::RunTest(
	const FString& Parameters)
{
	UNPCDecisionComponent* Decision =
		NewObject<UNPCDecisionComponent>();
	const FMissionContext Mission = MissionOverlayTests::MakeValidMission();

	TestTrue(TEXT("A valid Mission Context is accepted."),
		Decision->SetMissionContext(Mission));
	TestTrue(TEXT("Decision Component owns the active Mission projection."),
		Decision->HasActiveMission());
	TestEqual(TEXT("Mission identity is preserved."),
		Decision->GetActiveMissionContext().CommandId, Mission.CommandId);

	Decision->ClearMissionContext();
	TestFalse(TEXT("Clear removes the Mission projection."),
		Decision->HasActiveMission());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMissionOverlayRejectsInvalidContext,
	"Retry.Mission.Overlay.RejectsInvalidContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMissionOverlayRejectsInvalidContext::RunTest(const FString& Parameters)
{
	UNPCDecisionComponent* Decision =
		NewObject<UNPCDecisionComponent>();
	FMissionContext Mission = MissionOverlayTests::MakeValidMission();
	Mission.CommandId = FGuid();
	Mission.ObjectiveId = NAME_None;

	TestFalse(TEXT("An invalid Mission Context is rejected."),
		Decision->SetMissionContext(Mission));
	TestFalse(TEXT("Rejected input does not create active Mission state."),
		Decision->HasActiveMission());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMissionOverlayAllowsOnlyIdleAndPatrolMovement,
	"Retry.Mission.Overlay.AllowsOnlyIdleAndPatrolMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMissionOverlayAllowsOnlyIdleAndPatrolMovement::RunTest(
	const FString& Parameters)
{
	UNPCDecisionComponent* Decision =
		NewObject<UNPCDecisionComponent>();
	Decision->SetMissionContext(MissionOverlayTests::MakeValidMission());

	TestTrue(TEXT("Idle permits Mission movement."),
		Decision->IsMissionMovementAllowedForState(ENPCCombatState::Idle));
	TestTrue(TEXT("Patrol permits Mission movement."),
		Decision->IsMissionMovementAllowedForState(ENPCCombatState::Patrol));

	const ENPCCombatState InterruptingStates[] =
	{
		ENPCCombatState::Alert,
		ENPCCombatState::Search,
		ENPCCombatState::Attack,
		ENPCCombatState::TakeCover,
		ENPCCombatState::Reload,
		ENPCCombatState::Retreat,
		ENPCCombatState::Hold,
		ENPCCombatState::Suppress,
		ENPCCombatState::Dead,
	};

	for (const ENPCCombatState State : InterruptingStates)
	{
		TestFalse(
			*FString::Printf(TEXT("%s interrupts Mission movement."),
				*UEnum::GetValueAsString(State)),
			Decision->IsMissionMovementAllowedForState(State));
	}
	return true;
}

#endif
