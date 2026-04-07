// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTService_UpdatePersonality.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/PersonalityComponent.h"
#include "GameFramework/Character.h"

UBTService_UpdatePersonality::UBTService_UpdatePersonality()
{
	NodeName = TEXT("Update Personality");
	Interval = 0.5f;
	RandomDeviation = 0.1f;
}

void UBTService_UpdatePersonality::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return;

	ACharacter* NPC = Cast<ACharacter>(AIC->GetPawn());
	if (!NPC) return;

	UPersonalityComponent* PC =
		NPC->FindComponentByClass<UPersonalityComponent>();

	if (!PC) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	BB->SetValueAsFloat(TEXT("Aggression"), PC->GetAggression());
	BB->SetValueAsFloat(TEXT("Fear"), PC->GetFear());
	BB->SetValueAsFloat(TEXT("Trust"), PC->GetTrust());
}
