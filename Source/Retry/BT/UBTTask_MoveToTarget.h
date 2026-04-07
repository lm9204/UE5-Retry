// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UBTTask_MoveToTarget.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UUBTTask_MoveToTarget : public UBTTaskNode
{
	GENERATED_BODY()


public:
	UUBTTask_MoveToTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category="Movement")
	float AcceptanceRadius = 100.f;
};
