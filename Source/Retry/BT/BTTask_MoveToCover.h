// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToCover.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_MoveToCover : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToCover();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
	
};
