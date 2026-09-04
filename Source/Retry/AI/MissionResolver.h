#pragma once

#include "CoreMinimal.h"
#include "AI/CommandTypes.h"
#include "AI/ObservationPointSelector.h"

enum class EMissionResolutionOutcome : uint8
{
	Resolved,
	InvalidCommand,
	InvalidCommandState,
	UnsupportedCommand,
	ObjectiveMismatch,
	ObservationSelectionFailed,
};

struct FMissionResolutionResult
{
	EMissionResolutionOutcome Outcome =
		EMissionResolutionOutcome::InvalidCommand;
	EObservationPointSelectionOutcome SelectionOutcome =
		EObservationPointSelectionOutcome::InvalidObjectiveId;
	FMissionContext Mission;

	bool IsSuccess() const
	{
		return Outcome == EMissionResolutionOutcome::Resolved;
	}
};

class RETRY_API FMissionResolver
{
public:
	static FMissionResolutionResult ResolveReconArea(
		const FCommandIntent& Command,
		FName ResolvedObjectiveId,
		const TArray<FObservationPointCandidate>& Candidates);

	static FMissionResolutionResult ResolveSecureArea(
		const FCommandIntent& Command,
		FName ResolvedObjectiveId,
		FVector ResolvedObjectiveLocation);

	static FMissionResolutionResult ResolveDefendPosition(
		const FCommandIntent& Command,
		FVector ResolvedPosition);
};
