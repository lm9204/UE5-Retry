#pragma once

#include "CoreMinimal.h"
#include "AI/MissionResolver.h"

class AActor;
class UWorld;

enum class EReconMissionWorldOutcome : uint8
{
	Resolved,
	InvalidWorld,
	InvalidStartLocation,
	ObjectiveNotFound,
	DuplicateObjective,
	NavigationUnavailable,
	MissionResolutionFailed,
};

struct FReconMissionWorldResult
{
	EReconMissionWorldOutcome Outcome =
		EReconMissionWorldOutcome::InvalidWorld;
	FMissionResolutionResult Resolution;
	int32 CandidateCount = 0;

	bool IsSuccess() const
	{
		return Outcome == EReconMissionWorldOutcome::Resolved;
	}
};

class RETRY_API FReconMissionWorldAdapter
{
public:
	static FReconMissionWorldResult Resolve(
		UWorld* World,
		const FCommandIntent& Command,
		FVector StartLocation,
		AActor* PathfindingContext = nullptr);

	static FReconMissionWorldResult ResolveWithPathEvaluator(
		UWorld* World,
		const FCommandIntent& Command,
		TFunctionRef<bool(const FVector&, double&)> PathEvaluator);

	static FReconMissionWorldResult ResolveWithEvaluators(
		UWorld* World,
		const FCommandIntent& Command,
		TFunctionRef<bool(const FVector&, FVector&)> LocationProjector,
		TFunctionRef<bool(const FVector&, double&)> PathEvaluator);
};
