#pragma once

#include "CoreMinimal.h"
#include "Scenario/ScenarioDefinition.h"

class UTeamOperationalMemorySubsystem;

enum class EScenarioFollowUpReadiness : uint8
{
	Ready,
	WaitingForFacts,
	InvalidInput,
};

class RETRY_API FScenarioFollowUpOrderEvaluator
{
public:
	static EScenarioFollowUpReadiness Evaluate(
		const FScenarioFollowUpOrder& Order,
		uint8 TeamId,
		const FGuid& RunId,
		const UTeamOperationalMemorySubsystem* TeamMemory);

	static bool IsFirstPendingForGroup(
		const TArray<FScenarioFollowUpOrder>& PendingOrders,
		int32 OrderIndex);
};
