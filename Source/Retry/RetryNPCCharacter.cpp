// Fill out your copyright notice in the Description page of Project Settings.


#include "RetryNPCCharacter.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "FloatingNameWidget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CombatComponent.h"
#include "Components/HealthComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/PersonalityComponent.h"
#include "Components/WeaponComponent.h"
#include "Components/Widget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

// Sets default values
ARetryNPCCharacter::ARetryNPCCharacter()
{
	// 공유 컴포넌트
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	PersonalityComponent = CreateDefaultSubobject<UPersonalityComponent>(TEXT("PersonalityComponent"));
	WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));

	// AI Perception
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.f;
	SightConfig->LoseSightRadius = 2000.f;
	SightConfig->PeripheralVisionAngleDegrees = 90.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	PerceptionComponent->ConfigureSense(*SightConfig);

	//이름표 위젯
	NameplateWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameplateWidget"));
	NameplateWidget->SetupAttachment(RootComponent);
	NameplateWidget->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	NameplateWidget->SetWidgetSpace(EWidgetSpace::Screen);
}

// Called when the game starts or when spawned
void ARetryNPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[NPC] BeginPlay 호출"));

	// OnDeath 바인딩
	HealthComponent->OnDeath.AddDynamic(this, &ARetryNPCCharacter::OnDeath);
	HealthComponent->OnHealthChanged.AddDynamic(this, &ARetryNPCCharacter::OnHealthChanged);

	// Perception 바인딩
	if (PerceptionComponent)
	{
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
			this, &ARetryNPCCharacter::OnTargetPerceptionUpdated);
		UE_LOG(LogTemp, Warning, TEXT("[NPC] Perception 바인딩 완료"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[NPC] PerceptionComponent 없음"));
	}

	// FloatingName 설정
	if (FloatingNameWidgetClass)
	{
		NameplateWidget->SetWidgetClass(FloatingNameWidgetClass);

		if (UFloatingNameWidget* Widget =
			Cast<UFloatingNameWidget>(NameplateWidget->GetUserWidgetObject()))
		{
			Widget->SetNameText(NPCName);
			Widget->SetHPPercent(1.f);
		}
	}
}

void ARetryNPCCharacter::OnDeath()
{
	// BT 중단
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->BrainComponent->StopLogic(TEXT("Dead"));
	}

	GetCharacterMovement()->DisableMovement();
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetPhysicsBlendWeight(1.f);

	UE_LOG(LogTemp, Warning, TEXT("[NPC] %s is dead"), *GetName());
}

void ARetryNPCCharacter::OnHealthChanged(float CurrentHP, float MaxHP)
{
	if (UFloatingNameWidget* Widget =
		Cast<UFloatingNameWidget>(NameplateWidget->GetUserWidgetObject()))
	{
		Widget->SetHPPercent(CurrentHP / MaxHP);
	}
}

void ARetryNPCCharacter::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			if (Stimulus.WasSuccessfullySensed())
			{
				BB->SetValueAsObject(TEXT("TargetActor"), Actor);
				UE_LOG(LogTemp, Warning, TEXT("[NPC] 적 감지: %s"), *Actor->GetName());
			}
			else
			{
				BB->ClearValue(TEXT("TargetActor"));
			}
		}
	}
}
