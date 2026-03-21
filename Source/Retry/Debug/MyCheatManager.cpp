
#include "MyCheatManager.h"

#include "IPropertyTable.h"
#include "Components/CombatComponent.h"
#include "GameFramework/Character.h"
#include "Components/HealthComponent.h"
#include "Components/InventoryComponent.h"

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