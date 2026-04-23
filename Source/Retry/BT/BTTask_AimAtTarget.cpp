// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_AimAtTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_AimAtTarget::UBTTask_AimAtTarget()
{
	NodeName = TEXT("Aim At Target");
	TargetActorKey.AddObjectFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTTask_AimAtTarget, TargetActorKey),
		AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_AimAtTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	AActor* Target = Cast<AActor>(
		BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!Target) return EBTNodeResult::Failed;

	APawn* NPC = AIC->GetPawn();
	if (!NPC) return EBTNodeResult::Failed;

	//타겟 방향으로 회전
	FVector ToTarget = (Target->GetActorLocation() -
		NPC->GetActorLocation()).GetSafeNormal();
	FRotator LookRotation = ToTarget.Rotation();

	NPC->SetActorRotation(
		FMath::RInterpTo(NPC->GetActorRotation(), LookRotation,
			GetWorld()->GetDeltaSeconds(), 10.f));

	//허용 오차 내에 들어오면 성공
	float AngleDiff = FMath::Abs(
		FRotator::NormalizeAxis(
			LookRotation.Yaw - NPC->GetActorRotation().Yaw));

	if (AngleDiff <= AimTolerance)
		return EBTNodeResult::Succeeded;

	// 아직 에임 중이면 InProgress 반환
	return EBTNodeResult::InProgress;
}
