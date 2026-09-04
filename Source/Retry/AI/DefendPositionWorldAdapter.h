#pragma once

#include "CoreMinimal.h"
#include "AI/MissionResolver.h"

class AActor;
class UWorld;

enum class EDefendPositionWorldOutcome : uint8
{
	Resolved,
	InvalidWorld,
	InvalidStartLocation,
	InvalidTargetLocation,
	NavigationUnavailable,
	TargetProjectionFailed,
	PathUnavailable,
	MissionResolutionFailed,
};

struct FDefendPositionWorldResult
{
	EDefendPositionWorldOutcome Outcome =
		EDefendPositionWorldOutcome::InvalidWorld;
	FMissionResolutionResult Resolution;

	bool IsSuccess() const
	{
		return Outcome == EDefendPositionWorldOutcome::Resolved;
	}
};

class RETRY_API FDefendPositionWorldAdapter
{
public:
	static FDefendPositionWorldResult Resolve(
		UWorld* World,
		const FCommandIntent& Command,
		FVector StartLocation,
		AActor* PathfindingContext = nullptr);

	static FDefendPositionWorldResult ResolveWithEvaluators(
		UWorld* World,
		const FCommandIntent& Command,
		TFunctionRef<bool(const FVector&, FVector&)> LocationProjector,
		TFunctionRef<bool(const FVector&)> PathEvaluator);
};
