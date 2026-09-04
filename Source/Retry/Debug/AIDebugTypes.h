#pragma once

#include "CoreMinimal.h"
#include "AI/CommandTypes.h"
#include "AIDebugTypes.generated.h"

/** One-frame diagnostic view of Command, Mission, and Blackboard projection. */
USTRUCT(BlueprintType)
struct FAIMissionDebugSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Mission")
	bool bHasCommand = false;

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Mission")
	FString CommandId;

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Mission")
	ECommandVerb CommandVerb = ECommandVerb::Recon;

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Mission")
	ECommandStatus CommandStatus = ECommandStatus::Proposed;

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Mission")
	bool bHasMission = false;

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Mission")
	FName ObjectiveId;

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Mission")
	FVector MissionTargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Mission")
	bool bMissionMovementAllowed = false;

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Mission")
	bool bCommandMatchesMission = false;

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Blackboard")
	bool bBlackboardTargetSet = false;

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Blackboard")
	FVector BlackboardTargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Blackboard")
	bool bBlackboardMovementAllowed = false;

	UPROPERTY(BlueprintReadOnly, Category="AI Debug|Blackboard")
	bool bBlackboardSynchronized = false;
};

RETRY_API FAIMissionDebugSnapshot BuildAIMissionDebugSnapshot(
	bool bHasMission,
	const FMissionContext& Mission,
	bool bMissionMovementAllowed,
	const FCommandIntent* CurrentCommand,
	bool bBlackboardTargetSet,
	FVector BlackboardTargetLocation,
	bool bBlackboardMovementAllowed);
