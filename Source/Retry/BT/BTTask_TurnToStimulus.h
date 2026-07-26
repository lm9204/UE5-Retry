// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_TurnToStimulus.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_TurnToStimulus : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_TurnToStimulus();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
};
