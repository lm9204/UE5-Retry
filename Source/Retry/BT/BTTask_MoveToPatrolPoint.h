// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToPatrolPoint.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_MoveToPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()


public:
	UBTTask_MoveToPatrolPoint();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
};
