// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/UBTTask_MoveToTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UUBTTask_MoveToTarget::UUBTTask_MoveToTarget()
{
	NodeName = TEXT("Move To Target");
	TargetActorKey.AddObjectFilter(this,
		GET_MEMBER_NAME_CHECKED(UUBTTask_MoveToTarget, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UUBTTask_MoveToTarget::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!Target) return EBTNodeResult::Failed;

	FAIMoveRequest MoveReq(Target);
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);
	MoveReq.SetUsePathfinding(true);

	AIC->MoveTo(MoveReq);

	return EBTNodeResult::Succeeded;
}
