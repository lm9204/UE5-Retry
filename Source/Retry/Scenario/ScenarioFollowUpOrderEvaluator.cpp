#include "Scenario/ScenarioFollowUpOrderEvaluator.h"

#include "AI/TeamOperationalMemorySubsystem.h"

EScenarioFollowUpReadiness FScenarioFollowUpOrderEvaluator::Evaluate(
	const FScenarioFollowUpOrder& Order,
	const uint8 TeamId,
	const FGuid& RunId,
	const UTeamOperationalMemorySubsystem* TeamMemory)
{
	if (!RunId.IsValid() || !TeamMemory
		|| !Order.Command.CommandId.IsValid()
		|| Order.Command.AssignedGroupId.IsNone()
		|| Order.RequiredFacts.IsEmpty())
	{
		return EScenarioFollowUpReadiness::InvalidInput;
	}

	for (const FScenarioFactCondition& Condition : Order.RequiredFacts)
	{
		if (Condition.PredicateId.IsNone() || Condition.SubjectId.IsNone())
		{
			return EScenarioFollowUpReadiness::InvalidInput;
		}
		if (!TeamMemory->HasReceivedFact(
			TeamId,
			RunId,
			Condition.PredicateId,
			Condition.SubjectId,
			Condition.SourceGroupId))
		{
			return EScenarioFollowUpReadiness::WaitingForFacts;
		}
	}

	return EScenarioFollowUpReadiness::Ready;
}

bool FScenarioFollowUpOrderEvaluator::IsFirstPendingForGroup(
	const TArray<FScenarioFollowUpOrder>& PendingOrders,
	const int32 OrderIndex)
{
	if (!PendingOrders.IsValidIndex(OrderIndex))
	{
		return false;
	}

	const FName GroupId =
		PendingOrders[OrderIndex].Command.AssignedGroupId;
	if (GroupId.IsNone())
	{
		return false;
	}
	for (int32 Index = 0; Index < OrderIndex; ++Index)
	{
		if (PendingOrders[Index].Command.AssignedGroupId == GroupId)
		{
			return false;
		}
	}
	return true;
}
