// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTDecorator_HasPatrolPoints.h"

#include "AIController.h"
#include "RetryNPCCharacter.h"

UBTDecorator_HasPatrolPoints::UBTDecorator_HasPatrolPoints()
{
	NodeName = TEXT("Has Patrol Points");
}

bool UBTDecorator_HasPatrolPoints::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory) const
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return false;

	ARetryNPCCharacter* NPC = Cast<ARetryNPCCharacter>(AIC->GetPawn());
	if (!NPC) return false;

	return NPC->PatrolPoints.Num() > 0;
}
