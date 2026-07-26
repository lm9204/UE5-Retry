// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Overwatch.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_Overwatch : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Overwatch();

	UPROPERTY(EditAnywhere, Category="Overwatch")
	float WatchDuration = 3.f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
};
