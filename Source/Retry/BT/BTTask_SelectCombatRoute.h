// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SelectCombatRoute.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_SelectCombatRoute : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SelectCombatRoute();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	// 성격 수치 기반 이동 목표 위치 계산
	FVector GetAggressiveRoute(AActor* Target, APawn* NPC);
	FVector GetCautiousRoute(AActor* Target, APawn* NPC);
	FVector GetSupportiveRoute(APawn* NPC);
	FVector GetOpportunistRoute(AActor* Target, APawn* NPC,
		float Aggression, float Fear);
};
