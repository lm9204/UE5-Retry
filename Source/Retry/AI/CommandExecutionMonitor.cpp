#include "AI/CommandExecutionMonitor.h"

bool FCommandExecutionMonitor::IsWithinObservationRange(
	const FVector& ObserverLocation,
	const FVector& ObservationLocation,
	const float HorizontalRadius,
	const float VerticalTolerance)
{
	if (ObserverLocation.ContainsNaN()
		|| ObservationLocation.ContainsNaN()
		|| !FMath::IsFinite(HorizontalRadius)
		|| !FMath::IsFinite(VerticalTolerance)
		|| HorizontalRadius < 0.f
		|| VerticalTolerance < 0.f)
	{
		return false;
	}

	return FVector::DistSquared2D(ObserverLocation, ObservationLocation)
		<= FMath::Square(HorizontalRadius)
		&& FMath::Abs(ObserverLocation.Z - ObservationLocation.Z)
		<= VerticalTolerance;
}

FReconExecutionDecision FCommandExecutionMonitor::EvaluateRecon(
	const FCommandIntent& Command,
	const FReconExecutionSnapshot& Snapshot)
{
	FReconExecutionDecision Decision;
	if (Command.Status != ECommandStatus::Executing
		|| Command.Verb != ECommandVerb::Recon
		|| Command.TargetType != ECommandTargetType::Area
		|| !Command.CommandId.IsValid()
		|| !FMath::IsFinite(Snapshot.ExecutionElapsedSeconds)
		|| !FMath::IsFinite(Snapshot.StableObservationSeconds)
		|| Snapshot.ExecutionElapsedSeconds < 0.0
		|| Snapshot.StableObservationSeconds < 0.0)
	{
		return Decision;
	}

	if (!Snapshot.bLeaderAvailable)
	{
		Decision.Outcome =
			EReconExecutionOutcome::FailedLeaderUnavailable;
		return Decision;
	}

	const float TimeoutSeconds = Command.CompletionCriteria.TimeoutSeconds;
	if (TimeoutSeconds > 0.f
		&& Snapshot.ExecutionElapsedSeconds >= TimeoutSeconds)
	{
		Decision.Outcome = EReconExecutionOutcome::FailedTimeout;
		return Decision;
	}

	if (!Snapshot.bAtObservationPoint || !Snapshot.bObservationAllowed)
	{
		Decision.Outcome =
			EReconExecutionOutcome::WaitingForArrival;
		return Decision;
	}

	if (Snapshot.StableObservationSeconds
		< Command.CompletionCriteria.MinimumHoldSeconds)
	{
		Decision.Outcome =
			EReconExecutionOutcome::WaitingForObservationHold;
		return Decision;
	}

	Decision.Outcome = EReconExecutionOutcome::ObservationReady;
	return Decision;
}
