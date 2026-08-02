
#include "MyCheatManager.h"

#include "RetryNPCCharacter.h"
#include "RetryNPCController.h"
#include "Components/NPCDecisionComponent.h"
#include "Components/CombatComponent.h"
#include "GameFramework/Character.h"
#include "Components/HealthComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/MemoryComponent.h"
#include "Components/PersonalityComponent.h"
#include "Debug/CombatLogging.h"
#include "Engine/AssetManager.h"
#include "Items/ItemDefinition.h"
#include "Kismet/GameplayStatics.h"

void UMyCheatManager::DamageMe(float Amount)
{
	ACharacter* P = Cast<ACharacter>(GetOuterAPlayerController()->GetPawn());
	if (!P) return;

	UHealthComponent* HealthComponent = P->FindComponentByClass<UHealthComponent>();
	if (!HealthComponent) return;

	FDamageInfo Info;
	Info.BaseDamage = Amount;
	HealthComponent->TakeDamage(Info);
}

void UMyCheatManager::HealMe(float Amount)
{
	ACharacter* P = Cast<ACharacter>(GetOuterAPlayerController()->GetPawn());
	if (!P) return;

	UHealthComponent* HealthComponent = P->FindComponentByClass<UHealthComponent>();
	if (!HealthComponent) return;

	HealthComponent->Heal(Amount);
}

void UMyCheatManager::KillMe()
{
	DamageMe(99999.f);
}

void UMyCheatManager::TestAttack()
{
	ACharacter* P = Cast<ACharacter>(GetOuterAPlayerController()->GetPawn());
	if (!P) return;

	UCombatComponent* CC = P->FindComponentByClass<UCombatComponent>();
	if (!CC) return;

	CC->Attack();
}

// 기존 GiveItem은 그대로 두고 내부에서 재사용하도록 리팩토링
void UMyCheatManager::GiveItem(FString TypeName)
{
	APawn* Player = GetOuterAPlayerController()->GetPawn();
	if (!Player) return;

	GiveItemToActor(Player, TypeName, 1);
}

void UMyCheatManager::GiveItemToActor(
	AActor* TargetActor, FString TypeName, int32 Count)
{
	if (!TargetActor) return;

	UInventoryComponent* Inv =
		TargetActor->FindComponentByClass<UInventoryComponent>();
	if (!Inv) return;

	// UItemDefinition* Def = ItemDefinitionMap.FindRef(TypeName);
	// if (!Def)
	// {
	// 	UE_LOG(LogTemp, Error,
	// 		TEXT("[Cheat] ItemDefinition 없음: %s"), *TypeName);
	// 	return;
	// }
	//
	// FItemInstance Item;
	// Item.Definition = Def;
	// Item.StackCount = Count;

	// AssetManager에서 "Item:d" 형태로 조회
	FPrimaryAssetId Id("ItemDefinition", FName(*TypeName));

	UAssetManager& AM = UAssetManager::Get();
	FSoftObjectPath AssetPath = AM.GetPrimaryAssetPath(Id);

	if (!AssetPath.IsValid())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Cheat] ItemDefinition 없음: %s"), *TypeName);
		return;
	}

	UItemDefinition* Def = Cast<UItemDefinition>(AssetPath.TryLoad());
	if (!Def) return;

	FItemInstance Item;
	Item.Definition = Def;
	Item.StackCount = Count;

	Inv->AddItem(Item);
	UE_LOG(LogTemp, Warning,
		TEXT("[Cheat] %s에게 아이템 지급: %s x%d"),
		*GetCombatLogName(TargetActor), *TypeName, Count);
}

void UMyCheatManager::EquipItem(FName ItemID)
{
	ACharacter* P = Cast<ACharacter>(GetOuterAPlayerController()->GetPawn());
	if (!P) return;

	UInventoryComponent* Inv = P->FindComponentByClass<UInventoryComponent>();
	if (!Inv) return;

	Inv->EquipItem(ItemID);
}

void UMyCheatManager::UnEquipItem(ESlotType SlotType)
{
	ACharacter* P = Cast<ACharacter>(GetOuterAPlayerController()->GetPawn());
	if (!P) return;

	UInventoryComponent* Inv = P->FindComponentByClass<UInventoryComponent>();
	if (!Inv) return;

	Inv->UnEquipItem(SlotType);
}

void UMyCheatManager::TestArmorDamage(float Amount)
{
	ACharacter* P = Cast<ACharacter>(GetOuterAPlayerController()->GetPawn());
	if (!P) return;

	UInventoryComponent* Inv = P->FindComponentByClass<UInventoryComponent>();
	if (Inv)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Test] %s 현재 ArmorReduction: %.2f"),
			*GetCombatLogName(P), Inv->GetTotalArmorReduction());
	}

	DamageMe(Amount);
}


UInventoryComponent* UMyCheatManager::GetInventory()
{
	ACharacter* P = Cast<ACharacter>(GetOuterAPlayerController()->GetPawn());
	if (!P) return nullptr;

	return P->FindComponentByClass<UInventoryComponent>();
}

void UMyCheatManager::SetNPCAggression(float Value)
{
	// 가장 가까운 NPC 찾아서 수치 변경
	APawn* Player = GetOuterAPlayerController()->GetPawn();
	if (!Player) return;

	TArray<AActor*> NPCs;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), ARetryNPCCharacter::StaticClass(), NPCs);

	if (NPCs.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CheatManager] 주변에 NPC 없음"));
		return;
	}

	AActor* Closest = NPCs[0];
	for (AActor* NPC: NPCs)
	{
		if (FVector::Dist(Player->GetActorLocation(), NPC->GetActorLocation()) <
			FVector::Dist(Player->GetActorLocation(), Closest->GetActorLocation()))
		{
			Closest = NPC;
		}
	}

	if (UPersonalityComponent* PC =
		Closest->FindComponentByClass<UPersonalityComponent>())
	{
		PC->Aggression = FMath::Clamp(Value, 0.f, 1.f);
		PC->SyncToBlackboard();
		UE_LOG(LogTemp, Warning, TEXT("[CheatManager] %s Aggression -> %.2f"),
			*GetCombatLogName(Closest), PC->GetAggression());
	}
}

void UMyCheatManager::SpawnNPC(FString PersonalityType)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC) return;

	APawn* Player = PC->GetPawn();
	if (!Player) return;

	// 플레이어 앞 200 유닛에 스폰
	FVector SpawnLocation = Player->GetActorLocation()
		+ Player->GetActorForwardVector() * 200.f;

	FRotator SpawnRotation = Player->GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ARetryNPCCharacter* NPC = GetWorld()->SpawnActor<ARetryNPCCharacter>(
		NPCCharacterClass.Get(), SpawnLocation, SpawnRotation, SpawnParams);

	if (!IsValid(NPC)) 
	{
		UE_LOG(LogTemp, Error, TEXT("[Cheat] NPC 스폰 실패"));
		return;
	}

	UPersonalityComponent* PC_Comp =
		NPC->FindComponentByClass<UPersonalityComponent>();
	if (!PC_Comp) return;

	// 문자열로 성격 유형 설정
	if (PersonalityType.Equals(TEXT("Aggressive"), ESearchCase::IgnoreCase))
	{
		PC_Comp->DefaultPersonality = EPersonalityType::Aggressive;
	}
	else if (PersonalityType.Equals(TEXT("Cautious"), ESearchCase::IgnoreCase))
	{
		PC_Comp->DefaultPersonality = EPersonalityType::Cautious;
	}
	else if (PersonalityType.Equals(TEXT("Supportive"), ESearchCase::IgnoreCase))
	{
		PC_Comp->DefaultPersonality = EPersonalityType::Supportive;
	}
	else if (PersonalityType.Equals(TEXT("Opportunist"), ESearchCase::IgnoreCase))
	{
		PC_Comp->DefaultPersonality = EPersonalityType::Opportunist;
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Cheat] 알 수 없는 성격 유형: %s. Aggressive/Cautious/Supportive/Opportunist 중 입력"),
			*PersonalityType);
	}

	// 성격 프리셋 적용 + Blackboard 동기화
	PC_Comp->ApplyPersonalityPreset();
	PC_Comp->SyncToBlackboard();

	NPC->NPCName = PersonalityType;
	
	UE_LOG(LogTemp, Warning, TEXT("[Cheat] NPC 스폰: %s"), *PersonalityType);
}

void UMyCheatManager::ToggleAIDebug()
{
	TArray<AActor*> NPCs;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), ARetryNPCCharacter::StaticClass(), NPCs);

	for (AActor* Actor : NPCs)
	{
		ARetryNPCCharacter* NPC = Cast<ARetryNPCCharacter>(Actor);
		if (!NPC) continue;

		ARetryNPCController* Controller =
			Cast<ARetryNPCController>(NPC->GetController());
		if (!Controller || !Controller->DecisionComponent) continue;

		Controller->DecisionComponent->bDebugEnabled =
			!Controller->DecisionComponent->bDebugEnabled;

		NPC->AIDebugWidget->SetVisibility(
			Controller->DecisionComponent->bDebugEnabled);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Cheat] AI Debug UI 토글"));
}

void UMyCheatManager::TestCommandMemory()
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC) return;

	APawn* Player = PC->GetPawn();
	if (!Player) return;

	TArray<AActor*> NPCs;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), ARetryNPCCharacter::StaticClass(), NPCs);

	for (AActor* Actor : NPCs)
	{
		if (ARetryNPCCharacter* NPC = Cast<ARetryNPCCharacter>(Actor))
		{
			if (NPC->MemoryComponent)
			{
				NPC->MemoryComponent->AddMemory(
					EMemoryEventType::PlayerCommand,
					NPC->GetActorLocation(), 0.1f,
					TEXT("플레이어로부터 명령을 수신함"));
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Cheat] 전체 NPC에게 명령 메모리 기록"));
}

void UMyCheatManager::TriggerMemory(float Amount)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC) return;

	APawn* Player = PC->GetPawn();
	if (!Player) return;

	TArray<AActor*> NPCs;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), ARetryNPCCharacter::StaticClass(), NPCs);

	// 가장 가까운 고지능 NPC 찾기
	ARetryNPCCharacter* Target = nullptr;
	float MinDist = TNumericLimits<float>::Max();

	for (AActor* Actor : NPCs)
	{
		ARetryNPCCharacter* NPC = Cast<ARetryNPCCharacter>(Actor);
		if (!NPC || !NPC->bIsHighIntelligence) continue;

		float Dist = FVector::Dist(Player->GetActorLocation(), NPC->GetActorLocation());
		if (Dist < MinDist)
		{
			MinDist = Dist;
			Target = NPC;
		}
	}

	if (!Target)
	{
		UE_LOG(LogTemp, Error, TEXT("[Cheat] 고지능 NPC 없음"));
		return;
	}

	if (Target->MemoryComponent)
	{
		Target->MemoryComponent->AddMemory(
			EMemoryEventType::AllyDeath,
			Target->GetActorLocation(),
			Amount,
			TEXT("[테스트] 강제 트리거된 이벤트"));

		UE_LOG(LogTemp, Warning, TEXT("[Cheat] %s에게 EmotionWeight %.2f 강제 기록"),
			*GetCombatLogName(Target), Amount);
	}
}