#pragma once

#include "CoreMinimal.h"
#include "AI/CommandTypes.h"

enum class EAreaControlOutcome : uint8
{
	WaitingForEntry,
	Contested,
	HoldingControl,
	Secured,
	FailedLeaderUnavailable,
	FailedNoCombatPower,
	FailedTimeout,
	InvalidCommand,
};

struct FAreaControlSnapshot
{
	bool bLeaderAvailable = false;
	bool bLeaderInsideArea = false;
	int32 LivingGroupMemberCount = 0;
	int32 FriendlyCountInside = 0;
	int32 HostileCountInside = 0;
	double ExecutionElapsedSeconds = 0.0;
	double StableControlSeconds = 0.0;
};

struct FAreaControlDecision
{
	EAreaControlOutcome Outcome = EAreaControlOutcome::InvalidCommand;

	bool IsSecured() const
	{
		return Outcome == EAreaControlOutcome::Secured;
	}
};

class RETRY_API FAreaControlEvaluator
{
public:
	static FAreaControlDecision EvaluateSecureArea(
		const FCommandIntent& Command,
		const FAreaControlSnapshot& Snapshot);
};
