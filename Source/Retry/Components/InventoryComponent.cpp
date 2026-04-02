// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InventoryComponent.h"

#include "IDetailTreeNode.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

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

bool UInventoryComponent::AddItem(FItemData Item)
{
	// //무게 초과 체크
	// if (GetTotalWeight() + Item.Weight > MaxWeight)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("[Inventory] 무게 초과 - 아이템 추가 실패: %s"),
	// 		*Item.ItemID.ToString());
	// 	return false;
	// }

	Items.Add(Item);
	OnInventoryChanged.Broadcast();
	ApplyWeightPenalty();

	UE_LOG(LogTemp, Warning, TEXT("[Inventory] 아이템 추가: %s / 현재 무게: %.1f / %.1f"),
		*Item.ItemID.ToString(), GetTotalWeight(), MaxWeight);

	return true;
}

bool UInventoryComponent::RemoveItem(FName ItemID)
{
	int32 Index = Items.IndexOfByPredicate([&](const FItemData& Item)
	{
		return Item.ItemID == ItemID;
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
	FItemData* Item = FindItem(ItemID);
	if (!Item) return false;
	
	if (Item->SlotType == ESlotType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Inventory] 장착 불가 아이템: %s"),
			*ItemID.ToString());
		return false;
	}

	if (EquippedItems.Contains(Item->SlotType))
	{
		FItemData OldItem = EquippedItems[Item->SlotType];
		Items.Add(OldItem);
		
		// UEnum 통해서 DisplayName 가져오기
		const UEnum* EnumPtr = StaticEnum<ESlotType>();
		FString SlotName = EnumPtr->GetDisplayNameTextByValue((int64)Item->SlotType).ToString();
		
		UE_LOG(LogTemp, Warning, TEXT("[Inventory] 슬롯(%s) 교체: %s -> %s"),
			*SlotName, *OldItem.DisplayName.ToString(), *ItemID.ToString());
	}

	FItemData ItemCopy = *Item;
	RemoveItemInternal(ItemID);
	
	EquippedItems.Add(Item->SlotType, ItemCopy);
	OnInventoryChanged.Broadcast();
	
	const UEnum* EnumPtr = StaticEnum<ESlotType>();
	FString SlotName = EnumPtr->GetDisplayNameTextByValue((int64)ItemCopy.SlotType).ToString();
	UE_LOG(LogTemp, Warning, TEXT("[Inventory] 장착: %s → 슬롯: %s → ArmorReduction: %.2f"),
		*ItemID.ToString(), *SlotName, GetTotalArmorReduction());

	return true;
}

bool UInventoryComponent::UnEquipItem(ESlotType Slot)
{
	if (!EquippedItems.Contains(Slot)) return false;

	// 슬롯에서 꺼내서 인벤토리로 반환
	FItemData Item = EquippedItems[Slot];
	Items.Add(Item);

	EquippedItems.Remove(Slot);
	OnInventoryChanged.Broadcast();

	ApplyWeightPenalty();

	const UEnum* EnumPtr = StaticEnum<ESlotType>();
	FString SlotName = EnumPtr->GetDisplayNameTextByValue((int64)Slot).ToString();
	UE_LOG(LogTemp, Warning, TEXT("[Inventory] 해제 후 인벤토리 반환: %s ← %s"),
		*Item.ItemID.ToString(), *SlotName);
	
	return true;
}

float UInventoryComponent::GetTotalArmorReduction() const
{
	float Total = 0.f;

	for (const auto& Pair : EquippedItems)
	{
		Total += Pair.Value.ArmorReduction;
	}

	return FMath::Clamp(Total, 0.f, 0.8f);
}

float UInventoryComponent::GetTotalWeight() const
{
	float Total = 0.f;
	for (const FItemData& Item : Items)
	{
		Total += Item.Weight;
	}
	return Total;
}

bool UInventoryComponent::HasItem(FName ItemID) const
{
	return Items.ContainsByPredicate([&](const FItemData& Item)
	{
		return Item.ItemID == ItemID;
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

FItemData* UInventoryComponent::GetItemData(FName ItemID) const
{
	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[Inventory] ItemDataTable이 없음."));
		return nullptr;
	}

	return ItemDataTable->FindRow<FItemData>(ItemID, TEXT("GetItemData"));
}

FItemData* UInventoryComponent::FindItem(FName ItemID)
{
	return Items.FindByPredicate([&](const FItemData& Item)
	{
		return Item.ItemID == ItemID;
	});
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
	int32 Index = Items.IndexOfByPredicate([&](const FItemData& Item)
	{
		return Item.ItemID == ItemID;
	});
}
