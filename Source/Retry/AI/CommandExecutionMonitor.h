#pragma once

#include "CoreMinimal.h"
#include "AI/CommandTypes.h"

enum class EReconExecutionOutcome : uint8
{
	WaitingForArrival,
	WaitingForObservationHold,
	ObservationReady,
	FailedLeaderUnavailable,
	FailedTimeout,
	InvalidCommand,
};

struct FReconExecutionSnapshot
{
	bool bLeaderAvailable = false;
	bool bAtObservationPoint = false;
	bool bObservationAllowed = false;
	double ExecutionElapsedSeconds = 0.0;
	double StableObservationSeconds = 0.0;
};

struct FReconExecutionDecision
{
	EReconExecutionOutcome Outcome =
		EReconExecutionOutcome::InvalidCommand;

	bool IsObservationReady() const
	{
		return Outcome == EReconExecutionOutcome::ObservationReady;
	}
};

class RETRY_API FCommandExecutionMonitor
{
public:
	static bool IsWithinObservationRange(
		const FVector& ObserverLocation,
		const FVector& ObservationLocation,
		float HorizontalRadius,
		float VerticalTolerance);

	static FReconExecutionDecision EvaluateRecon(
		const FCommandIntent& Command,
		const FReconExecutionSnapshot& Snapshot);
};
