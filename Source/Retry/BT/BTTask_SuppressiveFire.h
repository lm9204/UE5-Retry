// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SuppressiveFire.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_SuppressiveFire : public UBTTaskNode
{
	GENERATED_BODY()


public:
	UBTTask_SuppressiveFire();

	UPROPERTY(EditAnywhere, Category="Combat")
	float SpreadAngle = 10.f;

	UPROPERTY(EditAnywhere, Category="Combat")
	int32 BurstCount = 5;
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
};
