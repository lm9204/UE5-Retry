// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToTarget.generated.h"

UENUM(BlueprintType)
enum class EMoveTargetType : uint8
{
	Actor    UMETA(DisplayName = "Actor (TargetActor 추적)"),
	Location UMETA(DisplayName = "Location (MoveDestination 이동)"),
};


/**
 * 
 */
UCLASS()
class RETRY_API UBTTask_MoveToTarget : public UBTTaskNode
{
	GENERATED_BODY()


public:
	UBTTask_MoveToTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	UPROPERTY(EditAnywhere, Category="Move Target")
	EMoveTargetType MoveTargetType = EMoveTargetType::Actor;
	
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector MoveDestinationKey;

	UPROPERTY(EditAnywhere, Category="Movement")
	float AcceptanceRadius = 100.f;
};
