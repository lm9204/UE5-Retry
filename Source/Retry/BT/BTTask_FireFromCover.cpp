// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_FireFromCover.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/HealthComponent.h"
#include "Components/WeaponComponent.h"
#include "GameFramework/Character.h"

UBTTask_FireFromCover::UBTTask_FireFromCover()
{
	NodeName = TEXT("Fire From Cover");
}

EBTNodeResult::Type UBTTask_FireFromCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	ACharacter* NPC = Cast<ACharacter>(AIC->GetPawn());
	if (!NPC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* Target = Cast<AActor>(
		BB->GetValueAsObject(TEXT("TargetActor")));
	if (!Target) return EBTNodeResult::Failed;

	UWeaponComponent* WC = NPC->FindComponentByClass<UWeaponComponent>();
	if (!WC || WC->GetCurrentAmmo() <= 0)
			return EBTNodeResult::Failed;

	if (UHealthComponent* TargetHC = Target->FindComponentByClass<UHealthComponent>())
	{
		if (TargetHC->IsDead())
			return EBTNodeResult::Failed;
	}

	// 타겟 방향으로 회전
	FVector ToTarget = Target->GetActorLocation() - NPC->GetActorLocation();
	ToTarget.Z = 0.f;
	FRotator LookAtRotation = ToTarget.Rotation();
	NPC->SetActorRotation(LookAtRotation);   // 즉시 회전 (스냅)

	WC->SetAimTarget(Target->GetActorLocation() + FVector(0, 0, 60.f));

	for (int32 i = 0; i < ShotsPerPeek; ++i)
		WC->Fire();

	return EBTNodeResult::Succeeded;
}
