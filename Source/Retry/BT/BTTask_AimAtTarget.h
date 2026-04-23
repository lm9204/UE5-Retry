// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AimAtTarget.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_AimAtTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AimAtTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	//허용 오차 각도
	UPROPERTY(EditAnywhere, Category="Aim")
	float AimTolerance = 10.f;

	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetActorKey;

};
