#pragma once

#include "CoreMinimal.h"
#include "AI/CommandTypes.h"
#include "AI/OperationalObjectiveTypes.h"

struct FCommanderGroupState
{
	FName GroupId;
	uint8 TeamId = 0;
	bool bIsAvailable = false;
};

enum class ECommanderPlanningOutcome : uint8
{
	Planned,
	InvalidInput,
	UnsupportedObjective,
	NoAvailableGroup,
};

struct FCommanderPlanningResult
{
	ECommanderPlanningOutcome Outcome =
		ECommanderPlanningOutcome::InvalidInput;
	FCommandIntent Command;

	bool IsSuccess() const
	{
		return Outcome == ECommanderPlanningOutcome::Planned;
	}
};

/** Deterministic doctrine baseline used before an LLM proposes Commands. */
class RETRY_API FCommanderPlanner
{
public:
	static FCommanderPlanningResult Plan(
		const FOperationalObjective& Objective,
		const TArray<FCommanderGroupState>& Groups,
		FGuid CommandId,
		FName IssuerId);
};
