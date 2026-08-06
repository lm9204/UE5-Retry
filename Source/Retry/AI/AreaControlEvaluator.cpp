#include "AI/AreaControlEvaluator.h"

FAreaControlDecision FAreaControlEvaluator::EvaluateSecureArea(
	const FCommandIntent& Command,
	const FAreaControlSnapshot& Snapshot)
{
	FAreaControlDecision Decision;
	if (Command.Status != ECommandStatus::Executing
		|| Command.Verb != ECommandVerb::Secure
		|| Command.TargetType != ECommandTargetType::Area
		|| !Command.CommandId.IsValid()
		|| !FMath::IsFinite(Snapshot.ExecutionElapsedSeconds)
		|| !FMath::IsFinite(Snapshot.StableControlSeconds)
		|| Snapshot.ExecutionElapsedSeconds < 0.0
		|| Snapshot.StableControlSeconds < 0.0
		|| Snapshot.LivingGroupMemberCount < 0
		|| Snapshot.FriendlyCountInside < 0
		|| Snapshot.HostileCountInside < 0
		|| !FMath::IsFinite(
			Command.CompletionCriteria.MinimumHoldSeconds)
		|| !FMath::IsFinite(
			Command.CompletionCriteria.TimeoutSeconds)
		|| Command.CompletionCriteria.MinimumHoldSeconds < 0.f
		|| Command.CompletionCriteria.TimeoutSeconds < 0.f
		|| (Command.CompletionCriteria.TimeoutSeconds > 0.f
			&& Command.CompletionCriteria.MinimumHoldSeconds
				> Command.CompletionCriteria.TimeoutSeconds))
	{
		return Decision;
	}

	if (!Snapshot.bLeaderAvailable)
	{
		Decision.Outcome = EAreaControlOutcome::FailedLeaderUnavailable;
		return Decision;
	}

	if (Snapshot.LivingGroupMemberCount == 0)
	{
		Decision.Outcome = EAreaControlOutcome::FailedNoCombatPower;
		return Decision;
	}

	const float TimeoutSeconds = Command.CompletionCriteria.TimeoutSeconds;
	if (TimeoutSeconds > 0.f
		&& Snapshot.ExecutionElapsedSeconds >= TimeoutSeconds)
	{
		Decision.Outcome = EAreaControlOutcome::FailedTimeout;
		return Decision;
	}

	if (!Snapshot.bLeaderInsideArea || Snapshot.FriendlyCountInside == 0)
	{
		Decision.Outcome = EAreaControlOutcome::WaitingForEntry;
		return Decision;
	}

	if (Snapshot.HostileCountInside > 0)
	{
		Decision.Outcome = EAreaControlOutcome::Contested;
		return Decision;
	}

	if (Snapshot.StableControlSeconds
		< Command.CompletionCriteria.MinimumHoldSeconds)
	{
		Decision.Outcome = EAreaControlOutcome::HoldingControl;
		return Decision;
	}

	Decision.Outcome = EAreaControlOutcome::Secured;
	return Decision;
}
