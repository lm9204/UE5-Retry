// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_ChargeEnemy.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTTask_ChargeEnemy::UBTTask_ChargeEnemy()
{
	NodeName = TEXT("Charge Enemy");
}

EBTNodeResult::Type UBTTask_ChargeEnemy::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* Target = Cast<AActor>(
		BB->GetValueAsObject(TEXT("TargetActor")));
	if (!Target) return EBTNodeResult::Failed;

	// 최대 속도로 적에게 돌진
	if (ACharacter* NPC = Cast<ACharacter>(AIC->GetPawn()))
	{
		NPC->GetCharacterMovement()->MaxWalkSpeed = 600.f;
	}

	AIC->MoveToActor(Target, 100.f);
	return EBTNodeResult::Succeeded;
}
