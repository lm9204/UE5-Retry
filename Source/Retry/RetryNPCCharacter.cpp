// Fill out your copyright notice in the Description page of Project Settings.


#include "RetryNPCCharacter.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "UI/FloatingNameWidget.h"
#include "LLMRequestQueue.h"
#include "AI/GroupManagerActor.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Components/DialogComponent.h"
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
#include "UI/DialogueWidget.h"

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
	DialogComponent = CreateDefaultSubobject<UDialogComponent>(TEXT("DialogComponent"));
	
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

	// Dialogue Widget
	DialogueWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(
	TEXT("DialogueWidgetComponent"));
	DialogueWidgetComponent->SetupAttachment(RootComponent);
	DialogueWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 500.f));  // NameplateWidget보다 위
	DialogueWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DialogueWidgetComponent->SetDrawSize(FVector2D(200.f, 80.f));
	DialogueWidgetComponent->SetVisibility(false);
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

	// Dialogue Widget
	if (bIsHighIntelligence && DialogueWidgetClass)
	{
		DialogueWidgetComponent->SetWidgetClass(DialogueWidgetClass);
		// DialogueWidgetComponent->SetVisibility(true);

		if (DialogComponent)
		{
			DialogComponent->OnDialogueRequested.AddDynamic(
				this, &ARetryNPCCharacter::OnDialogueRequested);
		}
	}

	// 그룹 등록
	if (MyGroup) MyGroup->RegisterMember(this, bIsGroupLeader);

	// NPC 인벤토리 아이템 지급
	if (StartingItems.Num() > 0)
	{
		for (UItemDefinition* Def : StartingItems)
		{
			if (!Def) continue;

			if (UInventoryComponent* Inv = InventoryComponent)
			{
				FItemInstance Item;
				Item.Definition = Def;
				Item.StackCount = 1;
				Inv->AddItem(Item);
				Inv->EquipItem(Def->ItemID);
			}
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
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// Capsule만 제거하고 Ragdoll Mesh는 정적/동적 바닥과 계속 충돌한다.
	// 전용 Projectile object channel만 무시해 시체 뒤의 타겟을 사격할 수 있다.
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetPhysicsBlendWeight(1.f);

	TArray<AActor*> AllNPCs;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), ARetryNPCCharacter::StaticClass(), AllNPCs);

	for (AActor* Actor : AllNPCs)
	{
		if (Actor == this) continue;

		ARetryNPCCharacter* OtherNPC = Cast<ARetryNPCCharacter>(Actor);
		if (!OtherNPC || !OtherNPC->MemoryComponent) continue;
		if (OtherNPC->TeamID != TeamID) continue;
		// if (!OtherNPC->bIsHighIntelligence) continue;
		// if (!OtherNPC->MyGroup || OtherNPC->MyGroup != MyGroup) continue;

		// 직접 목격 여부 판단 — 시야 체크
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(OtherNPC);

		FVector Start = OtherNPC->GetActorLocation() + FVector(0, 0, 60.f);
		FVector End = GetActorLocation() + FVector(0, 0, 60.f);
		float Dist = FVector::Dist(Start, End);

		bool bBlocked = GetWorld()->LineTraceSingleByChannel(
			Hit, Start, End, ECC_Visibility, Params);

		bool bWitnessed = !bBlocked && Dist <= 3000.f;

		if (bWitnessed)
		{
			// 같은 그룹원인 경우
			if (MyGroup && OtherNPC->MyGroup && OtherNPC->MyGroup == MyGroup)
			{
				MyGroup->AddGroupMemory(
					OtherNPC->NPCName,           // 목격자 ID
					TEXT("AllyDeath"),
					GetActorLocation(),
					0.3f,
					FString::Printf(TEXT("%s가 그룹 %s의 전투 중 사망을 직접 목격함"),
						*OtherNPC->NPCName, *NPCName)
				);
			}
			// 같은 그룹원은 아니지만 아군인 경우
			else if (MyGroup && OtherNPC->TeamID == TeamID)
			{
				OtherNPC->MyGroup->AddGroupMemory(
					OtherNPC->NPCName,           // 목격자 ID
					TEXT("AllyDeath"),
					GetActorLocation(),
					0.2f,
					FString::Printf(TEXT("%s가 아군 %s의 전투 중 사망을 직접 목격함"),
						*OtherNPC->NPCName, *NPCName)
				);
			}

			// 그룹에 소속되지 않은 아군인 경우
			if (!OtherNPC->MyGroup && OtherNPC->TeamID == TeamID)
			{
				OtherNPC->MemoryComponent->AddMemory(
					EMemoryEventType::AllyDeath,
					GetActorLocation(),
					0.2f,
					FString::Printf(TEXT("아군 %s가 전투 중 사망하는 것을 목격함"), *GetName())
				);
			}
		}
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

void ARetryNPCCharacter::OnDialogueRequested(const FString& Text, float Duration)
{
	if (!DialogueWidgetComponent) return;

	if (UDialogueWidget* Widget =
		Cast<UDialogueWidget>(DialogueWidgetComponent->GetUserWidgetObject()))
	{
		Widget->ShowText(Text, Duration);
	}
}
