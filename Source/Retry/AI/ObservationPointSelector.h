#pragma once

#include "CoreMinimal.h"

enum class EObservationPointSelectionOutcome : uint8
{
	Selected,
	InvalidObjectiveId,
	NoMatchingCandidates,
	NoUsableCandidates,
};

struct FObservationPointCandidate
{
	FName PointId;
	FName ObjectiveId;
	FVector Location = FVector::ZeroVector;
	bool bReachable = false;
	float UtilityScore = 0.f;
};

struct FObservationPointSelectionResult
{
	EObservationPointSelectionOutcome Outcome =
		EObservationPointSelectionOutcome::InvalidObjectiveId;
	FObservationPointCandidate SelectedCandidate;

	bool IsSuccess() const
	{
		return Outcome == EObservationPointSelectionOutcome::Selected;
	}
};

class RETRY_API FObservationPointSelector
{
public:
	static FObservationPointSelectionResult SelectBest(
		FName ObjectiveId,
		const TArray<FObservationPointCandidate>& Candidates);
};
