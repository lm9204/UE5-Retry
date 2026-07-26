// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FireFromCover.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_FireFromCover : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FireFromCover();

	UPROPERTY(EditAnywhere, Category="Combat")
	int32 ShotsPerPeek = 3;

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
};
