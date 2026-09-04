#include "Scenario/ScenarioOperationalObjectiveEvaluator.h"

EScenarioOperationalObjectiveReadiness
FScenarioOperationalObjectiveEvaluator::Evaluate(
	const FScenarioOperationalObjective& Objective,
	const FGuid& RunId,
	const TArray<FOperationalFact>& TeamFacts,
	FOperationalFact& OutAreaSecuredFact)
{
	OutAreaSecuredFact = FOperationalFact();
	if (!RunId.IsValid()
		|| Objective.ObjectiveId.IsNone()
		|| Objective.DesiredStateId
			!= OperationalObjectiveStates::MaintainAreaControl
		|| Objective.SubjectId.IsNone()
		|| Objective.Priority < 0
		|| Objective.Priority > 100
		|| Objective.ActivationFacts.IsEmpty())
	{
		return EScenarioOperationalObjectiveReadiness::InvalidInput;
	}

	for (const FScenarioFactCondition& Condition
		: Objective.ActivationFacts)
	{
		if (Condition.PredicateId.IsNone() || Condition.SubjectId.IsNone())
		{
			return EScenarioOperationalObjectiveReadiness::InvalidInput;
		}

		const FOperationalFact* MatchingFact = TeamFacts.FindByPredicate(
			[&Objective, &RunId, &Condition](const FOperationalFact& Fact)
			{
				return Fact.IsValid()
					&& Fact.TeamId == Objective.TeamId
					&& Fact.RunId == RunId
					&& Fact.PredicateId == Condition.PredicateId
					&& Fact.SubjectId == Condition.SubjectId
					&& (Condition.SourceGroupId.IsNone()
						|| Fact.SourceGroupId == Condition.SourceGroupId);
			});
		if (!MatchingFact)
		{
			return EScenarioOperationalObjectiveReadiness::WaitingForFacts;
		}
		if (Condition.PredicateId == OperationalPredicates::AreaSecured
			&& Condition.SubjectId == Objective.SubjectId)
		{
			OutAreaSecuredFact = *MatchingFact;
		}
	}

	return OutAreaSecuredFact.IsValid()
		? EScenarioOperationalObjectiveReadiness::Ready
		: EScenarioOperationalObjectiveReadiness::InvalidInput;
}
