// Fill out your copyright notice in the Description page of Project Settings.


#include "RetryNPCCharacter.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "FloatingNameWidget.h"
#include "LLMRequestQueue.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CombatComponent.h"
#include "Components/HealthComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/MemoryComponent.h"
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
	MemoryComponent = CreateDefaultSubobject<UMemoryComponent>(TEXT("MemoryComponent"));
	
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

	UE_LOG(LogTemp, Warning, TEXT("[NPC] %s BeginPlay 호출"), *NPCName);

	// OnDeath 바인딩
	HealthComponent->OnDeath.AddDynamic(this, &ARetryNPCCharacter::OnDeath);
	HealthComponent->OnHealthChanged.AddDynamic(this, &ARetryNPCCharacter::OnHealthChanged);
	HealthComponent->OnHitReaction.AddDynamic(this, &ARetryNPCCharacter::PlayHitReaction);

	// HitReaction 설정
	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		AnimInst->OnMontageEnded.AddDynamic(this, &ARetryNPCCharacter::OnHitMontageEnded);
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
	
			// if (!DefaultAmmo.IsEmpty())
			// {
			// 	CM->GiveItemToActor(this, DefaultAmmo,1);
			// 	CM->GiveItemToActor(this, DefaultAmmo,1);
			// 	CM->GiveItemToActor(this, DefaultAmmo,1);
			// 	InventoryComponent->EquipItem("ammo_556");
			// 	InventoryComponent->EquipItem("ammo_556");
			// 	InventoryComponent->EquipItem("ammo_556");
			// }
			
		}
	}

	if (MemoryComponent && bIsHighIntelligence)
	{
		MemoryComponent->OnMemoryThreshold.AddDynamic(this, &ARetryNPCCharacter::OnMemoryThresholdReached);
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

	TArray<AActor*> AllNPCs;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), ARetryNPCCharacter::StaticClass(), AllNPCs);

	for (AActor* Actor : AllNPCs)
	{
		if (Actor == this) return;

		ARetryNPCCharacter* OtherNPC = Cast<ARetryNPCCharacter>(Actor);
		if (!OtherNPC || !OtherNPC->MemoryComponent) continue;
		if (!OtherNPC->bIsHighIntelligence) continue;

		// 같은 팀만 (아군 사망 목격)
		if (OtherNPC->TeamID != TeamID) continue;

		float Dist = FVector::Dist(GetActorLocation(), OtherNPC->GetActorLocation());
		if (Dist > 3500.f) continue;  // 목격 범위

		OtherNPC->MemoryComponent->AddMemory(
			EMemoryEventType::AllyDeath,
			GetActorLocation(),
			0.4f,
			FString::Printf(TEXT("아군 %s가 전투 중 사망하는 것을 목격함"), *GetName())
		);
	}

	UE_LOG(LogTemp, Warning, TEXT("[NPC] %s is dead"), *NPCName);
}

void ARetryNPCCharacter::OnHealthChanged(float CurrentHP, float MaxHP)
{
	if (UFloatingNameWidget* Widget =
		Cast<UFloatingNameWidget>(NameplateWidget->GetUserWidgetObject()))
	{
		Widget->SetHPPercent(CurrentHP / MaxHP);
	}
}

void ARetryNPCCharacter::PlayHitReaction(FDamageInfo Info)
{
	if (!HitMontage) return;

	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		bIsStaggered = true;
		AnimInst->Montage_Play(HitMontage);
	}
}

void ARetryNPCCharacter::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == HitMontage)
	{
		bIsStaggered = false;
	}
}

void ARetryNPCCharacter::OnMemoryThresholdReached()
{
	if (!PersonalityComponent || !MemoryComponent) return;

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	ULLMRequestQueue* Queue = GI->GetSubsystem<ULLMRequestQueue>();
	if (!Queue) return;

	TArray<FNPCMemory> RecentMemories = MemoryComponent->GetRecentMemories(5);
	FString Prompt = PersonalityComponent->BuildLLMPrompt(RecentMemories);

	FLLMRequest Request;
	Request.TargetPersonality = PersonalityComponent;
	Request.TargetActor = this;
	Request.RequestType = ELLMRequestType::MemoryEvaluation;
	Request.Prompt = Prompt;
	Request.RequestTime = GetWorld()->GetTimeSeconds();

	Queue->Enqueue(Request);

	UE_LOG(LogTemp, Warning, TEXT("[NPC] %s LLM 요청 큐에 추가"), *NPCName);
}