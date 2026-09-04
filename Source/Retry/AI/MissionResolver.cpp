#include "AI/MissionResolver.h"

namespace MissionResolver
{
	void CopyMissionInputs(
		const FCommandIntent& Command,
		FMissionContext& Mission)
	{
		Mission.CommandId = Command.CommandId;
		Mission.InformationRequirements = Command.InformationRequirements;
		for (const FCommandConstraint& Constraint : Command.Constraints)
		{
			if (Constraint.bIsHardConstraint)
			{
				Mission.HardConstraints.Add(Constraint);
			}
			else
			{
				Mission.SoftConstraints.Add(Constraint);
			}
		}
	}
}

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

	MissionResolver::CopyMissionInputs(Command, Result.Mission);
	Result.Mission.ObjectiveId = Selection.SelectedCandidate.PointId;
	Result.Mission.ObjectiveLocation =
		Selection.SelectedCandidate.Location;

	Result.Outcome = EMissionResolutionOutcome::Resolved;
	return Result;
}

FMissionResolutionResult FMissionResolver::ResolveSecureArea(
	const FCommandIntent& Command,
	const FName ResolvedObjectiveId,
	const FVector ResolvedObjectiveLocation)
{
	FMissionResolutionResult Result;
	if (!Command.CommandId.IsValid() || Command.TargetId.IsNone()
		|| ResolvedObjectiveLocation.ContainsNaN())
	{
		return Result;
	}

	if (Command.Status != ECommandStatus::Assigned)
	{
		Result.Outcome = EMissionResolutionOutcome::InvalidCommandState;
		return Result;
	}

	if (Command.Verb != ECommandVerb::Secure
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

	MissionResolver::CopyMissionInputs(Command, Result.Mission);
	Result.Mission.ObjectiveId = ResolvedObjectiveId;
	Result.Mission.ObjectiveLocation = ResolvedObjectiveLocation;
	Result.Outcome = EMissionResolutionOutcome::Resolved;
	return Result;
}

FMissionResolutionResult FMissionResolver::ResolveDefendPosition(
	const FCommandIntent& Command,
	const FVector ResolvedPosition)
{
	FMissionResolutionResult Result;
	if (!Command.CommandId.IsValid() || ResolvedPosition.ContainsNaN())
	{
		return Result;
	}

	if (Command.Status != ECommandStatus::Assigned)
	{
		Result.Outcome = EMissionResolutionOutcome::InvalidCommandState;
		return Result;
	}

	if (Command.Verb != ECommandVerb::Defend
		|| Command.TargetType != ECommandTargetType::Position)
	{
		Result.Outcome = EMissionResolutionOutcome::UnsupportedCommand;
		return Result;
	}

	MissionResolver::CopyMissionInputs(Command, Result.Mission);
	Result.Mission.ObjectiveId = Command.TargetId.IsNone()
		? FName(TEXT("DefendPosition"))
		: Command.TargetId;
	Result.Mission.ObjectiveLocation = ResolvedPosition;
	Result.Outcome = EMissionResolutionOutcome::Resolved;
	return Result;
}
