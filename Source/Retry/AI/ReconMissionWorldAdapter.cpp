#include "AI/ReconMissionWorldAdapter.h"

#include "EngineUtils.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "AI/ObjectiveAreaActor.h"
#include "AI/ObservationPointActor.h"

FReconMissionWorldResult FReconMissionWorldAdapter::Resolve(
	UWorld* World,
	const FCommandIntent& Command,
	const FVector StartLocation,
	AActor* PathfindingContext)
{
	FReconMissionWorldResult Result;
	if (!IsValid(World))
	{
		return Result;
	}

	if (StartLocation.ContainsNaN())
	{
		Result.Outcome =
			EReconMissionWorldOutcome::InvalidStartLocation;
		return Result;
	}

	UNavigationSystemV1* NavigationSystem =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavigationSystem
		|| !NavigationSystem->GetDefaultNavDataInstance())
	{
		Result.Outcome =
			EReconMissionWorldOutcome::NavigationUnavailable;
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
		[World, StartLocation, PathfindingContext](
			const FVector& Destination,
			double& OutPathLength)
		{
			UNavigationPath* Path =
				UNavigationSystemV1::FindPathToLocationSynchronously(
					World,
					StartLocation,
					Destination,
					PathfindingContext);
			if (!Path || !Path->IsValid() || Path->IsPartial())
			{
				return false;
			}

			OutPathLength = Path->GetPathLength();
			return FMath::IsFinite(OutPathLength)
				&& OutPathLength >= 0.0;
		});
}

FReconMissionWorldResult
FReconMissionWorldAdapter::ResolveWithPathEvaluator(
	UWorld* World,
	const FCommandIntent& Command,
	TFunctionRef<bool(const FVector&, double&)> PathEvaluator)
{
	return ResolveWithEvaluators(
		World,
		Command,
		[](const FVector& MarkerLocation, FVector& OutMovementLocation)
		{
			OutMovementLocation = MarkerLocation;
			return !OutMovementLocation.ContainsNaN();
		},
		PathEvaluator);
}

FReconMissionWorldResult FReconMissionWorldAdapter::ResolveWithEvaluators(
	UWorld* World,
	const FCommandIntent& Command,
	TFunctionRef<bool(const FVector&, FVector&)> LocationProjector,
	TFunctionRef<bool(const FVector&, double&)> PathEvaluator)
{
	FReconMissionWorldResult Result;
	if (!IsValid(World))
	{
		return Result;
	}

	AObjectiveAreaActor* ResolvedObjective = nullptr;
	for (TActorIterator<AObjectiveAreaActor> It(World); It; ++It)
	{
		if (It->GetMarkerId() != Command.TargetId)
		{
			continue;
		}

		if (ResolvedObjective)
		{
			Result.Outcome =
				EReconMissionWorldOutcome::DuplicateObjective;
			return Result;
		}

		ResolvedObjective = *It;
	}

	if (!ResolvedObjective)
	{
		Result.Outcome =
			EReconMissionWorldOutcome::ObjectiveNotFound;
		return Result;
	}

	TArray<FObservationPointCandidate> Candidates;
	for (TActorIterator<AObservationPointActor> It(World); It; ++It)
	{
		if (It->GetObjectiveId() != ResolvedObjective->GetMarkerId())
		{
			continue;
		}

		FObservationPointCandidate& Candidate =
			Candidates.AddDefaulted_GetRef();
		Candidate.PointId = It->GetMarkerId();
		Candidate.ObjectiveId = It->GetObjectiveId();
		if (!LocationProjector(
			It->GetActorLocation(), Candidate.Location)
			|| Candidate.Location.ContainsNaN())
		{
			continue;
		}

		double PathLength = 0.0;
		Candidate.bReachable =
			PathEvaluator(Candidate.Location, PathLength)
			&& FMath::IsFinite(PathLength)
			&& PathLength >= 0.0
			&& PathLength <= TNumericLimits<float>::Max();
		if (Candidate.bReachable)
		{
			// Shorter reachable Nav paths receive a higher baseline score.
			Candidate.UtilityScore = -static_cast<float>(PathLength);
		}
	}

	Result.CandidateCount = Candidates.Num();
	Result.Resolution = FMissionResolver::ResolveReconArea(
		Command,
		ResolvedObjective->GetMarkerId(),
		Candidates);
	Result.Outcome = Result.Resolution.IsSuccess()
		? EReconMissionWorldOutcome::Resolved
		: EReconMissionWorldOutcome::MissionResolutionFailed;
	return Result;
}
