// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/PersonalityComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"

// Sets default values for this component's properties
UPersonalityComponent::UPersonalityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UPersonalityComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyPersonalityPreset();
	SyncToBlackboard();
}

EPersonalityType UPersonalityComponent::GetPersonalityType() const
{
	return DefaultPersonality;
}

void UPersonalityComponent::SyncToBlackboard()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner) return;

	AAIController* AIC = Cast<AAIController>(Owner->GetController());
	if (!AIC) return;

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB) return;

	BB->SetValueAsFloat(TEXT("Aggression"),  Aggression);
	BB->SetValueAsFloat(TEXT("Fear"),  Fear);
	BB->SetValueAsFloat(TEXT("Trust"),  Trust);
}

void UPersonalityComponent::ApplyPersonalityPreset()
{
	switch (DefaultPersonality)
	{
	case EPersonalityType::Aggressive:
		Aggression = 0.8f; Fear = 0.2f; Trust = 0.4f;
		break;
	case EPersonalityType::Cautious:
		Aggression = 0.2f; Fear = 0.7f; Trust = 0.5f;
		break;
	case EPersonalityType::Supportive:
		Aggression = 0.2f; Fear = 0.4f; Trust = 0.9f;
		break;
	case EPersonalityType::Opportunist:
		Aggression = 0.5f; Fear = 0.5f; Trust = 0.5f;
		break;
	}
}


