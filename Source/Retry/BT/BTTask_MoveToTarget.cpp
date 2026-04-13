// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_MoveToTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_MoveToTarget::UBTTask_MoveToTarget()
{
	NodeName = TEXT("Move To Target");
	TargetActorKey.AddObjectFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTTask_MoveToTarget, TargetActorKey), AActor::StaticClass());
	MoveDestinationKey.AddVectorFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTTask_MoveToTarget, MoveDestinationKey));
}

EBTNodeResult::Type UBTTask_MoveToTarget::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	if (MoveTargetType == EMoveTargetType::Actor)
	{
		AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
		if (!Target) return EBTNodeResult::Failed;

		AIC->MoveToActor(Target, AcceptanceRadius);
	}
	else
	{
		FVector Dest = BB->GetValueAsVector(
			MoveDestinationKey.SelectedKeyName);
		AIC->MoveToLocation(Dest, AcceptanceRadius);
	}

	return EBTNodeResult::Succeeded;
}
