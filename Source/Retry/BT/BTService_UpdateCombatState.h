// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateCombatState.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTService_UpdateCombatState : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateCombatState();

	UPROPERTY(EditAnywhere, Category="Combat")
	float LowHPThreshold = 0.3f;
	
	UPROPERTY(EditAnywhere, Category="Combat")
	float ClostRangeThreshold = 300.f;
	
	UPROPERTY(EditAnywhere, Category="Combat")
	float MidRangeThreshold = 800.f;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory, float DeltaSeconds) override;
};
