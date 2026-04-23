// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InventoryComponent.h"

#include "WeaponComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/AmmoFragment.h"
#include "Items/ArmorFragment.h"
#include "Items/ItemDefinition.h"
#include "Items/WeaponFragment.h"

// Sets default values
UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UInventoryComponent::AddItem(FItemInstance Item)
{
	if (!Item.IsValid()) return false;
	Items.Add(Item);
	OnInventoryChanged.Broadcast();
	ApplyWeightPenalty();

	UE_LOG(LogTemp, Warning, TEXT("[Inventory] 아이템 추가: %s / 현재 무게: %.1f / %.1f"),
		*Item.Definition->ItemID.ToString(), GetTotalWeight(), MaxWeight);

	return true;
}

FItemInstance* UInventoryComponent::FindItemByID(FName ItemID)
{
	return Items.FindByPredicate([&](const FItemInstance& I){
		return I.Definition && I.Definition->ItemID == ItemID;
	});
}

bool UInventoryComponent::RemoveItem(FName ItemID)
{
	int32 Index = Items.IndexOfByPredicate([&](const FItemInstance& Item)
	{
		return Item.Definition && Item.Definition->ItemID == ItemID;
	});

	if (Index == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Inventory] 아이템을 찾을 수 없음: %s"),
			*ItemID.ToString());
		return false;
	}

	Items.RemoveAt(Index);
	OnInventoryChanged.Broadcast();
	ApplyWeightPenalty();
	return true;
}

bool UInventoryComponent::EquipItem(FName ItemID)
{
	FItemInstance* Item = FindItemByID(ItemID);
	if (!Item || !Item->Definition) return false;

	// ArmorFragment
	if (UArmorFragment* Armor = Item->Definition->FindFragment<UArmorFragment>())
	{
		if (EquippedItems.Contains(Armor->SlotType))
		{
			FItemInstance PrevItem = EquippedItems[Armor->SlotType];
			Items.Add(PrevItem);

			// UEnum 통해서 DisplayName 가져오기
			const UEnum* EnumPtr = StaticEnum<ESlotType>();
			FString SlotName = EnumPtr->GetDisplayNameTextByValue((int64)Armor->SlotType).ToString();

			UE_LOG(LogTemp, Warning, TEXT("[Inventory] 슬롯(%s) 교체: %s -> %s"),
				*SlotName, *PrevItem.Definition->DisplayName.ToString(), *ItemID.ToString());
		}
		EquippedItems.Add(Armor->SlotType, *Item);
	}

	// WeaponFragment
	if (UWeaponFragment* Weapon = Item->Definition->FindFragment<UWeaponFragment>())
	{
		if (EquippedItems.Contains(ESlotType::Weapon))
		{
			FItemInstance PrevItem = EquippedItems[ESlotType::Weapon];
			Items.Add(PrevItem);

			// UEnum 통해서 DisplayName 가져오기
			const UEnum* EnumPtr = StaticEnum<ESlotType>();
			FString SlotName = EnumPtr->GetDisplayNameTextByValue((int64)ESlotType::Weapon).ToString();

			UE_LOG(LogTemp, Warning, TEXT("[Inventory] 슬롯(%s) 교체: %s -> %s"),
				*SlotName, *PrevItem.Definition->DisplayName.ToString(), *ItemID.ToString());
		}
		Weapon->OnFragmentActivated(GetOwner());
		EquippedItems.Add(ESlotType::Weapon, *Item);
	}

	// AmmoFragment
	if (UAmmoFragment* Ammo = Item->Definition->FindFragment<UAmmoFragment>())
	{
		UseAmmo(ItemID);
	}
	
	RemoveItemInternal(ItemID);
	OnInventoryChanged.Broadcast();

	ApplyWeightPenalty();

	return true;
}

bool UInventoryComponent::UnEquipItem(ESlotType Slot)
{
	if (!EquippedItems.Contains(Slot)) return false;

	// 슬롯에서 꺼내서 인벤토리로 반환
	FItemInstance Item = EquippedItems[Slot];
	Items.Add(Item);

	EquippedItems.Remove(Slot);
	OnInventoryChanged.Broadcast();

	ApplyWeightPenalty();

	const UEnum* EnumPtr = StaticEnum<ESlotType>();
	FString SlotName = EnumPtr->GetDisplayNameTextByValue((int64)Slot).ToString();
	UE_LOG(LogTemp, Warning, TEXT("[Inventory] 해제 후 인벤토리 반환: %s ← %s"),
		*Item.Definition->ItemID.ToString(), *SlotName);
	
	return true;
}

bool UInventoryComponent::UseAmmo(FName AmmoItemID)
{
	FItemInstance* Item = FindItemByID(AmmoItemID);
	if (!Item) return false;

	if (UAmmoFragment* AmmoFragment = Item->Definition->FindFragment<UAmmoFragment>())
	{
		if (AmmoFragment->AmmoCount <= 0) return false;

		if (UWeaponComponent* WC = GetOwner()->FindComponentByClass<UWeaponComponent>())
		{
			// if (!WC->IsArmed()) return false;
			WC->AddReserveAmmo(AmmoFragment->AmmoCount);
			
			return true;
		}
	}
	return false;
}

int32 UInventoryComponent::GetTotalAmmoCount(FName AmmoItemID)
{
	FItemInstance* Item = FindItemByID(AmmoItemID);
	return Item && Item->Definition ? Item->Definition->FindFragment<UAmmoFragment>()->AmmoCount : 0;
}

float UInventoryComponent::GetTotalArmorReduction() const
{
	float Total = 0.f;
	for (const auto& Pair : EquippedItems)
	{
		if (!Pair.Value.Definition) continue;
		if (UArmorFragment* ArmorFragment =
			Pair.Value.Definition->FindFragment<UArmorFragment>())
		{
			Total += ArmorFragment->ArmorReduction;
		}
	}

	return FMath::Clamp(Total, 0.f, 0.8f);
}

float UInventoryComponent::GetTotalWeight() const
{
	float Total = 0.f;
	// 인벤토리
	for (const FItemInstance& Item : Items)
	{
		UItemDefinition* ItemDefinition = Item.Definition;
		Total += ItemDefinition->Weight;
	}

	// 장착 아이템
	for (auto& Pair : EquippedItems)
	{
		UItemDefinition* ItemDefinition = Pair.Value.Definition;
		Total += ItemDefinition->Weight;
	}
	
	return Total;
}

bool UInventoryComponent::HasItem(FName ItemID) const
{
	return Items.ContainsByPredicate([&](const FItemInstance& Item)
	{
		return Item.Definition && Item.Definition->ItemID == ItemID;
	});
}

TArray<FEquippedItemSlot> UInventoryComponent::GetEquippedSlots() const
{
	TArray<FEquippedItemSlot> Result;
	for (const auto& Pair: EquippedItems)
	{
		FEquippedItemSlot S;
		S.Slot = Pair.Key;
		S.Item = Pair.Value;
		Result.Add(S);
	}
	return Result;
}

void UInventoryComponent::ApplyWeightPenalty()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner) return;

	UCharacterMovementComponent* Movement = Owner->GetCharacterMovement();
	if (!Movement) return;

	float WeightRatio = GetTotalWeight() / MaxWeight;

	if (WeightRatio > 0.8f)
	{
		Movement->MaxWalkSpeed = 250.f;
	}
	else
	{
		Movement->MaxWalkSpeed = 500.f;
	}
}

void UInventoryComponent::RemoveItemInternal(FName ItemID)
{
	int32 Index = Items.IndexOfByPredicate([&](const FItemInstance& Item)
	{
		return Item.Definition && Item.Definition->ItemID == ItemID;
	});

	Items.RemoveAt(Index);
}

void UInventoryComponent::ActivateItemFragment(FName ItemID, AActor* Owner)
{
	FItemInstance* Instance = FindItemByID(ItemID);
	if (!Instance || !Instance->Definition) return;

	for (UItemFragment* Fragment : Instance->Definition->Fragments)
	{
		if (Fragment) Fragment->OnFragmentActivated(Owner);
	}
}

void UInventoryComponent::DeactivateItemFragment(FName ItemID, AActor* Owner)
{
	FItemInstance* Instance = FindItemByID(ItemID);
	if (!Instance || !Instance->Definition) return;

	for (UItemFragment* Fragment : Instance->Definition->Fragments)
	{
		if (Fragment) Fragment->OnFragmentDeactivated(Owner);
	}
}
