// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_CloseDistance.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_CloseDistance::UBTTask_CloseDistance()
{
	NodeName = TEXT("Close Distance");
}

EBTNodeResult::Type UBTTask_CloseDistance::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* Target = Cast<AActor>(
		BB->GetValueAsObject(TEXT("TargetActor")));
	if (!Target) return EBTNodeResult::Failed;

	// 적 방향으로 TargetDistance까지 이동
	FVector Dir = (Target->GetActorLocation() - AIC->GetPawn()->GetActorLocation()).GetSafeNormal();
	FVector MoveTarget = Target->GetActorLocation() - Dir * TargetDistance;

	AIC->MoveToLocation(MoveTarget, 50.f);
	return EBTNodeResult::Succeeded;
}
