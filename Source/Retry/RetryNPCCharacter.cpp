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
#include "Components/WidgetComponent.h"
#include "Debug/AIDebugWidget.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/ItemDefinition.h"
#include "Debug/MyCheatManager.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ARetryNPCCharacter::ARetryNPCCharacter()
{
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	
	// 공유 컴포넌트
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	PersonalityComponent = CreateDefaultSubobject<UPersonalityComponent>(TEXT("PersonalityComponent"));
	WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));
	
	//이름표 위젯
	NameplateWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameplateWidget"));
	NameplateWidget->SetupAttachment(RootComponent);
	NameplateWidget->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	NameplateWidget->SetWidgetSpace(EWidgetSpace::Screen);

	// AIDebug Widget
	AIDebugWidget = CreateDefaultSubobject<UWidgetComponent>(
	TEXT("AIDebugWidget"));
	AIDebugWidget->SetupAttachment(RootComponent);
	AIDebugWidget->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	AIDebugWidget->SetWidgetSpace(EWidgetSpace::Screen);
	AIDebugWidget->SetVisibility(false);
}

// Called when the game starts or when spawned
void ARetryNPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[NPC] BeginPlay 호출"));

	// OnDeath 바인딩
	HealthComponent->OnDeath.AddDynamic(this, &ARetryNPCCharacter::OnDeath);
	HealthComponent->OnHealthChanged.AddDynamic(this, &ARetryNPCCharacter::OnHealthChanged);

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

	// AIDebug Widget
	if (AIDebugWidgetClass)
	{
		AIDebugWidget->SetWidgetClass(AIDebugWidgetClass);
	}

	APlayerController* PC =
		UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		if (UMyCheatManager* CM =
			Cast<UMyCheatManager>(PC->CheatManager))
		{
			if (!DefaultWeapon.IsEmpty())
			{
				CM->GiveItemToActor(this, DefaultWeapon, 1);
				InventoryComponent->EquipItem("m16a4");
			}

			if (!DefaultAmmo.IsEmpty())
			{
				CM->GiveItemToActor(this, DefaultAmmo,90);
				CM->GiveItemToActor(this, DefaultAmmo,90);
				CM->GiveItemToActor(this, DefaultAmmo,90);
				InventoryComponent->EquipItem("ammo_556");
			}
			
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
