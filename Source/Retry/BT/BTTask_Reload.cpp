// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_Reload.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/WeaponComponent.h"
#include "GameFramework/Character.h"

UBTTask_Reload::UBTTask_Reload()
{
	NodeName = TEXT("Reload");
}

EBTNodeResult::Type UBTTask_Reload::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	ACharacter* NPC = Cast<ACharacter>(AIC->GetPawn());
	if (!NPC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;
	
	UWeaponComponent* WC = NPC->FindComponentByClass<UWeaponComponent>();
	if (!WC) return EBTNodeResult::Failed;

	BB->SetValueAsBool(TEXT("bIsReloading"), true);
	WC->Reload();
	BB->SetValueAsBool(TEXT("bIsReloading"), false);

	return EBTNodeResult::Succeeded;
}
