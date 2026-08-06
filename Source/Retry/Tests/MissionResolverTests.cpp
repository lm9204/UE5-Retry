#include "Misc/AutomationTest.h"

#include "AI/MissionResolver.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace MissionResolverTests
{
	FCommandIntent MakeAssignedReconCommand()
	{
		FCommandIntent Command;
		Command.CommandId = FGuid::NewGuid();
		Command.IssuerId = TEXT("HQ");
		Command.AssignedGroupId = TEXT("ReconGroup");
		Command.Verb = ECommandVerb::Recon;
		Command.TargetType = ECommandTargetType::Area;
		Command.TargetId = TEXT("ReconArea_A");
		Command.Status = ECommandStatus::Assigned;
		return Command;
	}

	FObservationPointCandidate MakeCandidate(
		const FName PointId,
		const float Score,
		const bool bReachable = true)
	{
		FObservationPointCandidate Candidate;
		Candidate.PointId = PointId;
		Candidate.ObjectiveId = TEXT("ReconArea_A");
		Candidate.Location = FVector(Score, Score * 2.f, 0.f);
		Candidate.bReachable = bReachable;
		Candidate.UtilityScore = Score;
		return Candidate;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMissionResolverBuildsSecureAreaMission,
	"Retry.Mission.Resolver.BuildsSecureAreaMission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMissionResolverBuildsSecureAreaMission::RunTest(
	const FString& Parameters)
{
	FCommandIntent Command = MissionResolverTests::MakeAssignedReconCommand();
	Command.Verb = ECommandVerb::Secure;
	FCommandConstraint& Hard = Command.Constraints.AddDefaulted_GetRef();
	Hard.ConstraintId = TEXT("StayInsideArea");
	Hard.bIsHardConstraint = true;
	const FVector AreaLocation(500.f, 250.f, 10.f);

	const FMissionResolutionResult Result =
		FMissionResolver::ResolveSecureArea(
			Command, TEXT("ReconArea_A"), AreaLocation);

	TestTrue(TEXT("An assigned Secure Area command resolves."),
		Result.IsSuccess());
	TestEqual(TEXT("The Area remains the Mission objective."),
		Result.Mission.ObjectiveId, FName(TEXT("ReconArea_A")));
	TestEqual(TEXT("The resolved Area location is preserved."),
		Result.Mission.ObjectiveLocation, AreaLocation);
	TestEqual(TEXT("Hard constraints remain explicit."),
		Result.Mission.HardConstraints.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMissionResolverRejectsInvalidSecureArea,
	"Retry.Mission.Resolver.RejectsInvalidSecureArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMissionResolverRejectsInvalidSecureArea::RunTest(
	const FString& Parameters)
{
	FCommandIntent Command = MissionResolverTests::MakeAssignedReconCommand();
	Command.Verb = ECommandVerb::Secure;
	TestEqual(TEXT("A mismatched Objective is rejected."),
		FMissionResolver::ResolveSecureArea(
			Command, TEXT("OtherArea"), FVector::ZeroVector).Outcome,
		EMissionResolutionOutcome::ObjectiveMismatch);
	Command.Verb = ECommandVerb::Recon;
	TestEqual(TEXT("Recon cannot enter the Secure resolver."),
		FMissionResolver::ResolveSecureArea(
			Command, TEXT("ReconArea_A"), FVector::ZeroVector).Outcome,
		EMissionResolutionOutcome::UnsupportedCommand);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMissionResolverBuildsReconMissionContext,
	"Retry.Mission.Resolver.BuildsReconMissionContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMissionResolverBuildsReconMissionContext::RunTest(
	const FString& Parameters)
{
	FCommandIntent Command = MissionResolverTests::MakeAssignedReconCommand();

	FCommandConstraint HardConstraint;
	HardConstraint.ConstraintId = TEXT("MustBeReachable");
	HardConstraint.bIsHardConstraint = true;
	Command.Constraints.Add(HardConstraint);

	FCommandConstraint SoftConstraint;
	SoftConstraint.ConstraintId = TEXT("PreferLowExposure");
	SoftConstraint.bIsHardConstraint = false;
	Command.Constraints.Add(SoftConstraint);

	FInformationRequirement Requirement;
	Requirement.RequirementId = TEXT("EnemyPresence");
	Requirement.SubjectId = TEXT("ReconArea_A");
	Command.InformationRequirements.Add(Requirement);

	const TArray<FObservationPointCandidate> Candidates =
	{
		MissionResolverTests::MakeCandidate(TEXT("ReconObs_A1"), 10.f),
		MissionResolverTests::MakeCandidate(TEXT("ReconObs_A2"), 20.f),
	};

	const FMissionResolutionResult Result =
		FMissionResolver::ResolveReconArea(
			Command, TEXT("ReconArea_A"), Candidates);

	TestTrue(TEXT("An assigned Recon Area command resolves."),
		Result.IsSuccess());
	TestEqual(TEXT("The Command identity is preserved."),
		Result.Mission.CommandId, Command.CommandId);
	TestEqual(TEXT("The selected Point becomes the execution objective."),
		Result.Mission.ObjectiveId, FName(TEXT("ReconObs_A2")));
	TestEqual(TEXT("Hard constraints remain hard."),
		Result.Mission.HardConstraints.Num(), 1);
	TestEqual(TEXT("Soft constraints remain soft."),
		Result.Mission.SoftConstraints.Num(), 1);
	TestEqual(TEXT("Information requirements are preserved."),
		Result.Mission.InformationRequirements.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMissionResolverRejectsWrongBoundaryInputs,
	"Retry.Mission.Resolver.RejectsWrongBoundaryInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMissionResolverRejectsWrongBoundaryInputs::RunTest(
	const FString& Parameters)
{
	FCommandIntent Command = MissionResolverTests::MakeAssignedReconCommand();
	const FObservationPointCandidate Candidate =
		MissionResolverTests::MakeCandidate(TEXT("ReconObs_A1"), 10.f);

	Command.Status = ECommandStatus::Proposed;
	const FMissionResolutionResult WrongState =
		FMissionResolver::ResolveReconArea(
			Command, TEXT("ReconArea_A"), { Candidate });
	TestEqual(TEXT("Only an assigned command can resolve."),
		WrongState.Outcome,
		EMissionResolutionOutcome::InvalidCommandState);

	Command.Status = ECommandStatus::Assigned;
	Command.Verb = ECommandVerb::Secure;
	const FMissionResolutionResult WrongVerb =
		FMissionResolver::ResolveReconArea(
			Command, TEXT("ReconArea_A"), { Candidate });
	TestEqual(TEXT("This resolver only accepts Recon Area."),
		WrongVerb.Outcome,
		EMissionResolutionOutcome::UnsupportedCommand);

	Command.Verb = ECommandVerb::Recon;
	const FMissionResolutionResult WrongObjective =
		FMissionResolver::ResolveReconArea(
			Command, TEXT("OtherArea"), { Candidate });
	TestEqual(TEXT("The resolved Objective must match the Command target."),
		WrongObjective.Outcome,
		EMissionResolutionOutcome::ObjectiveMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMissionResolverPreservesSelectionFailure,
	"Retry.Mission.Resolver.PreservesSelectionFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMissionResolverPreservesSelectionFailure::RunTest(
	const FString& Parameters)
{
	const FCommandIntent Command =
		MissionResolverTests::MakeAssignedReconCommand();
	const FObservationPointCandidate BlockedCandidate =
		MissionResolverTests::MakeCandidate(
			TEXT("ReconObs_A1"), 10.f, false);

	const FMissionResolutionResult Result =
		FMissionResolver::ResolveReconArea(
			Command, TEXT("ReconArea_A"), { BlockedCandidate });

	TestEqual(TEXT("Resolver reports selection failure."),
		Result.Outcome,
		EMissionResolutionOutcome::ObservationSelectionFailed);
	TestEqual(TEXT("The detailed selection reason is preserved."),
		Result.SelectionOutcome,
		EObservationPointSelectionOutcome::NoUsableCandidates);
	return true;
}

#endif
