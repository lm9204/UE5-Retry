#pragma once

#include "CoreMinimal.h"
#include "AI/MissionResolver.h"

class AActor;
class UWorld;

enum class ESecureAreaWorldOutcome : uint8
{
	Resolved,
	InvalidWorld,
	InvalidStartLocation,
	ObjectiveNotFound,
	DuplicateObjective,
	InvalidAreaRadius,
	NavigationUnavailable,
	ObjectiveProjectionFailed,
	PathUnavailable,
	MissionResolutionFailed,
};

struct FSecureAreaWorldResult
{
	ESecureAreaWorldOutcome Outcome = ESecureAreaWorldOutcome::InvalidWorld;
	FMissionResolutionResult Resolution;
	float AreaRadius = 0.f;

	bool IsSuccess() const
	{
		return Outcome == ESecureAreaWorldOutcome::Resolved;
	}
};

class RETRY_API FSecureAreaWorldAdapter
{
public:
	static FSecureAreaWorldResult Resolve(
		UWorld* World,
		const FCommandIntent& Command,
		FVector StartLocation,
		AActor* PathfindingContext = nullptr);

	static FSecureAreaWorldResult ResolveWithEvaluators(
		UWorld* World,
		const FCommandIntent& Command,
		TFunctionRef<bool(const FVector&, FVector&)> LocationProjector,
		TFunctionRef<bool(const FVector&)> PathEvaluator);
};
