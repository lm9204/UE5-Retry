#include "AI/SecureAreaWorldAdapter.h"

#include "EngineUtils.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "AI/ObjectiveAreaActor.h"

FSecureAreaWorldResult FSecureAreaWorldAdapter::Resolve(
	UWorld* World,
	const FCommandIntent& Command,
	const FVector StartLocation,
	AActor* PathfindingContext)
{
	FSecureAreaWorldResult Result;
	if (!IsValid(World))
	{
		return Result;
	}
	if (StartLocation.ContainsNaN())
	{
		Result.Outcome = ESecureAreaWorldOutcome::InvalidStartLocation;
		return Result;
	}

	UNavigationSystemV1* NavigationSystem =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavigationSystem
		|| !NavigationSystem->GetDefaultNavDataInstance())
	{
		Result.Outcome = ESecureAreaWorldOutcome::NavigationUnavailable;
		return Result;
	}

	return ResolveWithEvaluators(
		World,
		Command,
		[NavigationSystem](
			const FVector& MarkerLocation,
			FVector& OutMovementLocation)
		{
			FNavLocation ProjectedLocation;
			if (!NavigationSystem->ProjectPointToNavigation(
				MarkerLocation, ProjectedLocation))
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

FSecureAreaWorldResult FSecureAreaWorldAdapter::ResolveWithEvaluators(
	UWorld* World,
	const FCommandIntent& Command,
	TFunctionRef<bool(const FVector&, FVector&)> LocationProjector,
	TFunctionRef<bool(const FVector&)> PathEvaluator)
{
	FSecureAreaWorldResult Result;
	if (!IsValid(World))
	{
		return Result;
	}

	AObjectiveAreaActor* Objective = nullptr;
	for (TActorIterator<AObjectiveAreaActor> It(World); It; ++It)
	{
		if (It->GetMarkerId() != Command.TargetId)
		{
			continue;
		}
		if (Objective)
		{
			Result.Outcome = ESecureAreaWorldOutcome::DuplicateObjective;
			return Result;
		}
		Objective = *It;
	}

	if (!Objective)
	{
		Result.Outcome = ESecureAreaWorldOutcome::ObjectiveNotFound;
		return Result;
	}
	if (!FMath::IsFinite(Objective->GetAreaRadius())
		|| Objective->GetAreaRadius() <= 0.f)
	{
		Result.Outcome = ESecureAreaWorldOutcome::InvalidAreaRadius;
		return Result;
	}

	FVector MovementLocation;
	if (!LocationProjector(Objective->GetActorLocation(), MovementLocation)
		|| MovementLocation.ContainsNaN())
	{
		Result.Outcome = ESecureAreaWorldOutcome::ObjectiveProjectionFailed;
		return Result;
	}
	if (!PathEvaluator(MovementLocation))
	{
		Result.Outcome = ESecureAreaWorldOutcome::PathUnavailable;
		return Result;
	}

	Result.AreaRadius = Objective->GetAreaRadius();
	Result.Resolution = FMissionResolver::ResolveSecureArea(
		Command, Objective->GetMarkerId(), MovementLocation);
	Result.Outcome = Result.Resolution.IsSuccess()
		? ESecureAreaWorldOutcome::Resolved
		: ESecureAreaWorldOutcome::MissionResolutionFailed;
	return Result;
}
