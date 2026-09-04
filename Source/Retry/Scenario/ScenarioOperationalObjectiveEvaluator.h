#pragma once

#include "CoreMinimal.h"
#include "AI/OperationalTypes.h"
#include "Scenario/ScenarioDefinition.h"

enum class EScenarioOperationalObjectiveReadiness : uint8
{
	Ready,
	WaitingForFacts,
	InvalidInput,
};

class RETRY_API FScenarioOperationalObjectiveEvaluator
{
public:
	static EScenarioOperationalObjectiveReadiness Evaluate(
		const FScenarioOperationalObjective& Objective,
		const FGuid& RunId,
		const TArray<FOperationalFact>& TeamFacts,
		FOperationalFact& OutAreaSecuredFact);
};
