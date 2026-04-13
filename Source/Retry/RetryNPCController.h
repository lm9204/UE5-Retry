// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "RetryNPCController.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API ARetryNPCController : public AAIController
{
	GENERATED_BODY()

public:
	ARetryNPCController(const FObjectInitializer& ObjectInitializer);

	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(1);
	}

	UPROPERTY(EditDefaultsOnly, Category="AI")
	class UBehaviorTree* BehaviorTreeAsset;

protected:
	virtual void OnPossess(APawn* InPawn) override;
};
