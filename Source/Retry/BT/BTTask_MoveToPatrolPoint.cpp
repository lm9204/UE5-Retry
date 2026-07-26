// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_MoveToPatrolPoint.h"

#include "AIController.h"
#include "RetryNPCCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"

class ARetryNPCCharacter;

UBTTask_MoveToPatrolPoint::UBTTask_MoveToPatrolPoint()
{
	NodeName = TEXT("Move To Patrol Point");
}

EBTNodeResult::Type UBTTask_MoveToPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	ARetryNPCCharacter* NPC =
		Cast<ARetryNPCCharacter>(AIC->GetPawn());
	if (!NPC || NPC->PatrolPoints.Num() == 0)
		return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;
	int32 Index = BB->GetValueAsInt(TEXT("PatrolIndex"));

	FVector Target = NPC->PatrolPoints[Index]->GetActorLocation();
	AIC->MoveToLocation(Target, 50.f);

	// 다음 인덱스로
	int32 Next = (Index + 1) % NPC->PatrolPoints.Num();
	BB->SetValueAsInt(TEXT("PatrolIndex"), Next);

	return EBTNodeResult::Succeeded;
}
