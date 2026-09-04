#include "AI/CommanderPlanner.h"

#include "AI/CommandValidation.h"

FCommanderPlanningResult FCommanderPlanner::Plan(
	const FOperationalObjective& Objective,
	const TArray<FCommanderGroupState>& Groups,
	const FGuid CommandId,
	const FName IssuerId)
{
	FCommanderPlanningResult Result;
	if (!Objective.IsValid() || !CommandId.IsValid() || IssuerId.IsNone())
	{
		return Result;
	}
	if (Objective.DesiredStateId
		!= OperationalObjectiveStates::MaintainAreaControl)
	{
		Result.Outcome = ECommanderPlanningOutcome::UnsupportedObjective;
		return Result;
	}

	TArray<FName> AvailableGroupIds;
	for (const FCommanderGroupState& Group : Groups)
	{
		if (Group.bIsAvailable
			&& Group.TeamId == Objective.TeamId
			&& !Group.GroupId.IsNone())
		{
			AvailableGroupIds.AddUnique(Group.GroupId);
		}
	}
	AvailableGroupIds.Sort(
		[](const FName Left, const FName Right)
		{
			return Left.ToString() < Right.ToString();
		});
	if (AvailableGroupIds.IsEmpty())
	{
		Result.Outcome = ECommanderPlanningOutcome::NoAvailableGroup;
		return Result;
	}

	Result.Command.CommandId = CommandId;
	Result.Command.IssuerId = IssuerId;
	Result.Command.AssignedGroupId = AvailableGroupIds[0];
	Result.Command.Verb = ECommandVerb::Defend;
	Result.Command.TargetType = ECommandTargetType::Position;
	Result.Command.TargetId = Objective.SubjectId;
	Result.Command.TargetLocation = Objective.TargetLocation;
	Result.Command.Priority = Objective.Priority;
	Result.Command.Status = ECommandStatus::Proposed;

	if (!FCommandValidator::Validate(Result.Command).IsValid())
	{
		Result.Command = FCommandIntent();
		return Result;
	}

	Result.Outcome = ECommanderPlanningOutcome::Planned;
	return Result;
}
