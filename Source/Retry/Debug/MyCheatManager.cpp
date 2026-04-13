
#include "MyCheatManager.h"

#include "IPropertyTable.h"
#include "RetryNPCCharacter.h"
#include "RetryNPCController.h"
#include "Components/CombatComponent.h"
#include "GameFramework/Character.h"
#include "Components/HealthComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/PersonalityComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionSystem.h"

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

void UMyCheatManager::GiveItem(FString TypeName)
{
	UInventoryComponent* Inv = GetInventory();
	if (!Inv || !Inv->ItemDataTable) return;

	EItemType TargetType;
	if      (TypeName == "Armor")      TargetType = EItemType::Armor;
	else if (TypeName == "Weapon")     TargetType = EItemType::Weapon;
	else if (TypeName == "Consumable") TargetType = EItemType::Consumable;
	else
	{
		// 타입 아니면 ID로 직접 조회
		FItemData* Data = Inv->GetItemData(FName(*TypeName));
		if (Data) Inv->AddItem(*Data);
		else UE_LOG(LogTemp, Error, TEXT("[Cheat] 없는 아이템: %s"), *TypeName);
		return;
	}

	TArray<FItemData*> Rows;
	Inv->ItemDataTable->GetAllRows<FItemData>(TEXT("GiveItem"), Rows);

	for (FItemData* Row : Rows)
	{
		if (Row->ItemType == TargetType)
		{
			Inv->AddItem(*Row);
			UE_LOG(LogTemp, Warning, TEXT("[Cheat] 지급: %s"), *Row->ItemID.ToString());
		}
	}
}

void UMyCheatManager::GiveArmor(float Reduction)
{
	ACharacter* P = Cast<ACharacter>(GetOuterAPlayerController()->GetPawn());
	if (!P) return;

	UInventoryComponent* Inv = P->FindComponentByClass<UInventoryComponent>();
	if (!Inv) return;

	FItemData Armor;
	Armor.ItemID			= FName("test_armor");
	Armor.ItemType			= EItemType::Armor;
	Armor.ArmorReduction	= FMath::Clamp(Reduction, 0.f, 0.8f);
	Armor.Weight			= 5.f;

	Inv->AddItem(Armor);
	Inv->EquipItem(FName("test_armor"));
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
		UE_LOG(LogTemp, Warning, TEXT("[Test] 현재 ArmorReduction: %.2f"),
			Inv->GetTotalArmorReduction());
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
			*Closest->GetName(), PC->GetAggression());
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
