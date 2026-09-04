#pragma once

#include "CoreMinimal.h"
#include "AI/OperationalTypes.h"
#include "OperationalObjectiveTypes.generated.h"

namespace OperationalObjectiveStates
{
	extern const FName MaintainAreaControl;
}

/** A desired operational state that may produce one or more Commands. */
USTRUCT(BlueprintType)
struct FOperationalObjective
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational Objective")
	FGuid ObjectiveInstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational Objective")
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational Objective")
	FGuid SourceFactId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational Objective")
	FName ObjectiveId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational Objective")
	FName DesiredStateId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational Objective")
	FName SubjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational Objective")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational Objective")
	uint8 TeamId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational Objective",
		meta=(ClampMin="0", ClampMax="100"))
	int32 Priority = 50;

	bool IsValid() const;
};

RETRY_API bool BuildMaintainAreaControlObjective(
	FGuid ObjectiveInstanceId,
	FName ObjectiveId,
	int32 Priority,
	const FOperationalFact& AreaSecuredFact,
	FOperationalObjective& OutObjective,
	FText& OutError);
