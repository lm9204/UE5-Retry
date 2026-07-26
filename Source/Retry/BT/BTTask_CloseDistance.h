// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CloseDistance.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_CloseDistance : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_CloseDistance();

	UPROPERTY(EditAnywhere, Category="Combat")
	float TargetDistance = 500.f;
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
};
