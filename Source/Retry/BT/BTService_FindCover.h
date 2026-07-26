// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_FindCover.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UBTService_FindCover : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_FindCover();

	UPROPERTY(EditAnywhere, Category="Cover")
	float SearchRadius = 800.f;

	UPROPERTY(EditAnywhere, Category="Cover")
	int32 NumTestPoints = 16;
	
protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory, float DeltaSeconds) override;

private:
	bool IsInCover(FVector TestPoint, FVector EnemyLocation, APawn* NPC);
};
