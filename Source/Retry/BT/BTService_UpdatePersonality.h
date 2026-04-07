// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdatePersonality.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTService_UpdatePersonality : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdatePersonality();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory, float DeltaSeconds) override;
};
