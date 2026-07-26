// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CallOut.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_CallOut : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_CallOut();

	UPROPERTY(EditAnywhere, Category="CallOut")
	float AlertRadius = 1000.f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
};
