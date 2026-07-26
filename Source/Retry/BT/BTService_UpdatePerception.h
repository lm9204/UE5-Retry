// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdatePerception.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTService_UpdatePerception : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdatePerception();

	UPROPERTY(EditAnywhere, Category="Perception")
	float SightRange = 1500.f;

	UPROPERTY(EditAnywhere, Category="Perception")
	float HearingRange = 800.f;
	
protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory, float DeltaSeconds) override;
};
