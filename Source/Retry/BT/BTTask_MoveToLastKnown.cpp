// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_MoveToLastKnown.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_MoveToLastKnown::UBTTask_MoveToLastKnown()
{
	NodeName = TEXT("Move To Last Known Location");
}

EBTNodeResult::Type UBTTask_MoveToLastKnown::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	FVector LastKnown = BB->GetValueAsVector(TEXT("LastKnownEnemyLocation"));

	if (LastKnown.IsZero()) return EBTNodeResult::Aborted;

	AIC->MoveToLocation(LastKnown, 100.f);
	return EBTNodeResult::Succeeded;
}
