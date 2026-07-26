// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTService_UpdateCombatState.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/HealthComponent.h"
#include "Components/WeaponComponent.h"
#include "GameFramework/Character.h"

UBTService_UpdateCombatState::UBTService_UpdateCombatState()
{
	NodeName = TEXT("Update Combat State");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

void UBTService_UpdateCombatState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return;

	ACharacter* NPC = Cast<ACharacter>(AIC->GetPawn());
	if (!NPC) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	// HP 상태
	if (UHealthComponent* HC = NPC->FindComponentByClass<UHealthComponent>())
	{
		float HPRatio = HC->GetCurrentHealth() / HC->MaxHealth;
		BB->SetValueAsBool(TEXT("bIsLowHP"), HPRatio <= LowHPThreshold);
	}

	// 탄약 상태
	if (UWeaponComponent* WC = NPC->FindComponentByClass<UWeaponComponent>())
	{
		BB->SetValueAsBool(TEXT("bIsOutOfAmmo"), WC->GetCurrentAmmo() <= 0);
		BB->SetValueAsBool(TEXT("bIsReloading"), WC->GetIsReloading());
	}

	//적과의 거리
	AActor* Target = Cast<AActor>(
		BB->GetValueAsObject(TEXT("TargetActor")));

	if (Target)
	{
		float Dist = FVector::Dist(
			NPC->GetActorLocation(),
			Target->GetActorLocation());
		BB->SetValueAsFloat(TEXT("DistanceToTarget"), Dist);

		// 마지막 감지 위치 업데이트 (시야 있을 때만)
		bool bCanSee = BB->GetValueAsBool(TEXT("bCanSeeTarget"));
		if (bCanSee)
		{
			BB->SetValueAsVector(TEXT("LastKnownEnemyLocation"),
				Target->GetActorLocation());	
		}
	}
	else
	{
		BB->SetValueAsFloat(TEXT("DistanceToTarget"), 99999.f);
	}
}
