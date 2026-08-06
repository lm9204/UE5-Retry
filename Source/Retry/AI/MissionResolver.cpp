#include "AI/MissionResolver.h"

FMissionResolutionResult FMissionResolver::ResolveReconArea(
	const FCommandIntent& Command,
	const FName ResolvedObjectiveId,
	const TArray<FObservationPointCandidate>& Candidates)
{
	FMissionResolutionResult Result;
	if (!Command.CommandId.IsValid() || Command.TargetId.IsNone())
	{
		return Result;
	}

	if (Command.Status != ECommandStatus::Assigned)
	{
		Result.Outcome = EMissionResolutionOutcome::InvalidCommandState;
		return Result;
	}

	if (Command.Verb != ECommandVerb::Recon
		|| Command.TargetType != ECommandTargetType::Area)
	{
		Result.Outcome = EMissionResolutionOutcome::UnsupportedCommand;
		return Result;
	}

	if (ResolvedObjectiveId.IsNone()
		|| ResolvedObjectiveId != Command.TargetId)
	{
		Result.Outcome = EMissionResolutionOutcome::ObjectiveMismatch;
		return Result;
	}

	const FObservationPointSelectionResult Selection =
		FObservationPointSelector::SelectBest(
			ResolvedObjectiveId, Candidates);
	Result.SelectionOutcome = Selection.Outcome;
	if (!Selection.IsSuccess())
	{
		Result.Outcome =
			EMissionResolutionOutcome::ObservationSelectionFailed;
		return Result;
	}

	Result.Mission.CommandId = Command.CommandId;
	Result.Mission.ObjectiveId = Selection.SelectedCandidate.PointId;
	Result.Mission.ObjectiveLocation =
		Selection.SelectedCandidate.Location;
	Result.Mission.InformationRequirements =
		Command.InformationRequirements;

	for (const FCommandConstraint& Constraint : Command.Constraints)
	{
		if (Constraint.bIsHardConstraint)
		{
			Result.Mission.HardConstraints.Add(Constraint);
		}
		else
		{
			Result.Mission.SoftConstraints.Add(Constraint);
		}
	}

	Result.Outcome = EMissionResolutionOutcome::Resolved;
	return Result;
}
