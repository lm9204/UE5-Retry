#include "AI/ObservationPointSelector.h"

namespace
{
	bool IsCandidateDataValid(const FObservationPointCandidate& Candidate)
	{
		return !Candidate.PointId.IsNone()
			&& !Candidate.Location.ContainsNaN()
			&& FMath::IsFinite(Candidate.UtilityScore);
	}

	bool HasDeterministicPriority(
		const FObservationPointCandidate& Candidate,
		const FObservationPointCandidate& CurrentBest)
	{
		if (!FMath::IsNearlyEqual(
			Candidate.UtilityScore,
			CurrentBest.UtilityScore))
		{
			return Candidate.UtilityScore > CurrentBest.UtilityScore;
		}

		return Candidate.PointId.ToString().Compare(
			CurrentBest.PointId.ToString(),
			ESearchCase::CaseSensitive) < 0;
	}
}

FObservationPointSelectionResult FObservationPointSelector::SelectBest(
	const FName ObjectiveId,
	const TArray<FObservationPointCandidate>& Candidates)
{
	FObservationPointSelectionResult Result;
	if (ObjectiveId.IsNone())
	{
		return Result;
	}

	bool bFoundMatchingCandidate = false;
	bool bFoundUsableCandidate = false;

	for (const FObservationPointCandidate& Candidate : Candidates)
	{
		if (Candidate.ObjectiveId != ObjectiveId)
		{
			continue;
		}

		bFoundMatchingCandidate = true;
		if (!Candidate.bReachable || !IsCandidateDataValid(Candidate))
		{
			continue;
		}

		if (!bFoundUsableCandidate
			|| HasDeterministicPriority(
				Candidate, Result.SelectedCandidate))
		{
			Result.SelectedCandidate = Candidate;
			bFoundUsableCandidate = true;
		}
	}

	if (bFoundUsableCandidate)
	{
		Result.Outcome = EObservationPointSelectionOutcome::Selected;
	}
	else if (bFoundMatchingCandidate)
	{
		Result.Outcome =
			EObservationPointSelectionOutcome::NoUsableCandidates;
	}
	else
	{
		Result.Outcome =
			EObservationPointSelectionOutcome::NoMatchingCandidates;
	}

	return Result;
}
