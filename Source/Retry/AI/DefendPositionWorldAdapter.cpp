#include "AI/DefendPositionWorldAdapter.h"

#include "NavigationPath.h"
#include "NavigationSystem.h"

FDefendPositionWorldResult FDefendPositionWorldAdapter::Resolve(
	UWorld* World,
	const FCommandIntent& Command,
	const FVector StartLocation,
	AActor* PathfindingContext)
{
	FDefendPositionWorldResult Result;
	if (!IsValid(World))
	{
		return Result;
	}
	if (StartLocation.ContainsNaN())
	{
		Result.Outcome = EDefendPositionWorldOutcome::InvalidStartLocation;
		return Result;
	}

	UNavigationSystemV1* NavigationSystem =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavigationSystem
		|| !NavigationSystem->GetDefaultNavDataInstance())
	{
		Result.Outcome =
			EDefendPositionWorldOutcome::NavigationUnavailable;
		return Result;
	}

	return ResolveWithEvaluators(
		World,
		Command,
		[NavigationSystem](
			const FVector& TargetLocation,
			FVector& OutMovementLocation)
		{
			FNavLocation ProjectedLocation;
			if (!NavigationSystem->ProjectPointToNavigation(
				TargetLocation, ProjectedLocation))
			{
				return false;
			}
			OutMovementLocation = ProjectedLocation.Location;
			return !OutMovementLocation.ContainsNaN();
		},
		[World, StartLocation, PathfindingContext](const FVector& Destination)
		{
			UNavigationPath* Path =
				UNavigationSystemV1::FindPathToLocationSynchronously(
					World, StartLocation, Destination, PathfindingContext);
			return Path && Path->IsValid() && !Path->IsPartial();
		});
}

FDefendPositionWorldResult
FDefendPositionWorldAdapter::ResolveWithEvaluators(
	UWorld* World,
	const FCommandIntent& Command,
	TFunctionRef<bool(const FVector&, FVector&)> LocationProjector,
	TFunctionRef<bool(const FVector&)> PathEvaluator)
{
	FDefendPositionWorldResult Result;
	if (!IsValid(World))
	{
		return Result;
	}
	if (Command.TargetLocation.ContainsNaN())
	{
		Result.Outcome =
			EDefendPositionWorldOutcome::InvalidTargetLocation;
		return Result;
	}

	FVector MovementLocation;
	if (!LocationProjector(Command.TargetLocation, MovementLocation)
		|| MovementLocation.ContainsNaN())
	{
		Result.Outcome =
			EDefendPositionWorldOutcome::TargetProjectionFailed;
		return Result;
	}
	if (!PathEvaluator(MovementLocation))
	{
		Result.Outcome = EDefendPositionWorldOutcome::PathUnavailable;
		return Result;
	}

	Result.Resolution =
		FMissionResolver::ResolveDefendPosition(Command, MovementLocation);
	Result.Outcome = Result.Resolution.IsSuccess()
		? EDefendPositionWorldOutcome::Resolved
		: EDefendPositionWorldOutcome::MissionResolutionFailed;
	return Result;
}
