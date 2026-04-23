// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_FireAtTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/WeaponComponent.h"
#include "GameFramework/Character.h"

UBTTask_FireAtTarget::UBTTask_FireAtTarget()
{
	NodeName = TEXT("Fire At Target");
	TargetActorKey.AddObjectFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTTask_FireAtTarget, TargetActorKey),
		AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_FireAtTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	ACharacter* NPC = Cast<ACharacter>(AIC->GetPawn());
	if (!NPC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	AActor* Target = Cast<AActor>(
		BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!Target) return EBTNodeResult::Failed;

	//사거리 체크
	float Distance = FVector::Dist(
		NPC->GetActorLocation(), Target->GetActorLocation());
	if (Distance > MaxFireRange) return EBTNodeResult::Failed;

	// WeaponComponent로 발사
	UWeaponComponent* WeaponComp = NPC->FindComponentByClass<UWeaponComponent>();
	if (!WeaponComp || !WeaponComp->IsArmed())
	{
		return EBTNodeResult::Failed;
	}

	WeaponComp->Fire();

	return EBTNodeResult::Succeeded;
}
