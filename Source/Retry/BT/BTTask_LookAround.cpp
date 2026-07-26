// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_LookAround.h"

#include "AIController.h"

UBTTask_LookAround::UBTTask_LookAround()
{
	NodeName = TEXT("Look Around");
}

EBTNodeResult::Type UBTTask_LookAround::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	APawn* NPC = AIC->GetPawn();
	if (!NPC) return EBTNodeResult::Failed;

	// 랜덤 방향으로 두리번
	float RandYaw = FMath::RandRange(-180.f, 180.f);
	FVector LookDir = FRotator(0.f, NPC->GetActorRotation().Yaw + RandYaw, 0.f).Vector();
	FVector LookTarget = NPC->GetActorLocation() + LookDir * LookingRange;

	AIC->SetFocalPoint(LookTarget);
	return EBTNodeResult::Succeeded;
}
