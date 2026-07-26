// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToLastKnown.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_MoveToLastKnown : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToLastKnown();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
	
};
