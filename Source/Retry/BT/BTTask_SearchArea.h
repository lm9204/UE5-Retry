// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SearchArea.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_SearchArea : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SearchArea();

	UPROPERTY(EditAnywhere, Category="Search")
	float SearchRadius = 500.f;

	UPROPERTY(EditAnywhere, Category="Search")
	float SearchDuration = 10.f;

	UPROPERTY(EditAnywhere, Category="Search")
	float SearchPointPerSecond = 2.f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
	
};
