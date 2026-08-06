#include "Misc/AutomationTest.h"

#include "AI/ObservationPointSelector.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ObservationPointSelectorTests
{
	FObservationPointCandidate MakeCandidate(
		const FName PointId,
		const FName ObjectiveId,
		const float UtilityScore,
		const bool bReachable = true)
	{
		FObservationPointCandidate Candidate;
		Candidate.PointId = PointId;
		Candidate.ObjectiveId = ObjectiveId;
		Candidate.Location = FVector(UtilityScore, 0.f, 0.f);
		Candidate.bReachable = bReachable;
		Candidate.UtilityScore = UtilityScore;
		return Candidate;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FObservationPointSelectorChoosesHighestUsableScore,
	"Retry.Mission.ObservationSelector.ChoosesHighestUsableScore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FObservationPointSelectorChoosesHighestUsableScore::RunTest(
	const FString& Parameters)
{
	const TArray<FObservationPointCandidate> Candidates =
	{
		ObservationPointSelectorTests::MakeCandidate(
			TEXT("ReconObs_A1"), TEXT("ReconArea_A"), 10.f),
		ObservationPointSelectorTests::MakeCandidate(
			TEXT("ReconObs_A2"), TEXT("ReconArea_A"), 20.f),
		ObservationPointSelectorTests::MakeCandidate(
			TEXT("OtherObs"), TEXT("OtherArea"), 100.f),
		ObservationPointSelectorTests::MakeCandidate(
			TEXT("BlockedObs"), TEXT("ReconArea_A"), 200.f, false),
	};

	const FObservationPointSelectionResult Result =
		FObservationPointSelector::SelectBest(
			TEXT("ReconArea_A"), Candidates);

	TestTrue(TEXT("A usable candidate is selected."), Result.IsSuccess());
	TestEqual(TEXT("The highest usable score wins."),
		Result.SelectedCandidate.PointId, FName(TEXT("ReconObs_A2")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FObservationPointSelectorUsesStableIdTieBreak,
	"Retry.Mission.ObservationSelector.UsesStableIdTieBreak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FObservationPointSelectorUsesStableIdTieBreak::RunTest(
	const FString& Parameters)
{
	const TArray<FObservationPointCandidate> Candidates =
	{
		ObservationPointSelectorTests::MakeCandidate(
			TEXT("ReconObs_A2"), TEXT("ReconArea_A"), 20.f),
		ObservationPointSelectorTests::MakeCandidate(
			TEXT("ReconObs_A1"), TEXT("ReconArea_A"), 20.f),
	};

	const FObservationPointSelectionResult Result =
		FObservationPointSelector::SelectBest(
			TEXT("ReconArea_A"), Candidates);

	TestEqual(TEXT("Equal scores use the stable Point ID order."),
		Result.SelectedCandidate.PointId, FName(TEXT("ReconObs_A1")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FObservationPointSelectorReportsFailureReasons,
	"Retry.Mission.ObservationSelector.ReportsFailureReasons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FObservationPointSelectorReportsFailureReasons::RunTest(
	const FString& Parameters)
{
	const FObservationPointCandidate BlockedCandidate =
		ObservationPointSelectorTests::MakeCandidate(
			TEXT("ReconObs_A1"), TEXT("ReconArea_A"), 20.f, false);

	const FObservationPointSelectionResult InvalidObjective =
		FObservationPointSelector::SelectBest(NAME_None, { BlockedCandidate });
	TestEqual(TEXT("An empty Objective ID is rejected."),
		InvalidObjective.Outcome,
		EObservationPointSelectionOutcome::InvalidObjectiveId);

	const FObservationPointSelectionResult NoMatch =
		FObservationPointSelector::SelectBest(
			TEXT("OtherArea"), { BlockedCandidate });
	TestEqual(TEXT("A missing Objective link is distinguished."),
		NoMatch.Outcome,
		EObservationPointSelectionOutcome::NoMatchingCandidates);

	const FObservationPointSelectionResult NoUsable =
		FObservationPointSelector::SelectBest(
			TEXT("ReconArea_A"), { BlockedCandidate });
	TestEqual(TEXT("An unreachable matching candidate is distinguished."),
		NoUsable.Outcome,
		EObservationPointSelectionOutcome::NoUsableCandidates);
	return true;
}

#endif
