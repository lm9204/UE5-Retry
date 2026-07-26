// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasPatrolPoints.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTDecorator_HasPatrolPoints : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_HasPatrolPoints();

protected:
	virtual bool CalculateRawConditionValue(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) const override;
};
