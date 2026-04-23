// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/LootComponent.h"

#include "Actor/DroppedItemActor.h"
#include "InventoryComponent.h"
#include "Items/ItemDefinition.h"

// Sets default values for this component's properties
ULootComponent::ULootComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void ULootComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerInventory = GetOwner()->FindComponentByClass<UInventoryComponent>();
	if (!OwnerInventory)
	{
		UE_LOG(LogTemp, Error, TEXT("[Loot] InventoryComponent 없음"));
	}
}

bool ULootComponent::LootItem(ADroppedItemActor* DroppedItem)
{
	if (!DroppedItem || !OwnerInventory) return false;

	FItemInstance Data = DroppedItem->GetItemData();
	bool bSuccess = OwnerInventory->AddItem(Data);

	if (bSuccess)
	{
		NearbyItems.Remove(DroppedItem);
		OnLootRangeChanged.Broadcast(NearbyItems);
		DroppedItem->Destroy();

		UE_LOG(LogTemp, Warning, TEXT("[Loot] 획득: %s"), *Data.Definition->ItemID.ToString());
	}

	return bSuccess;
}

void ULootComponent::LootAll()
{
	// 복사본으로 순회
	TArray<ADroppedItemActor*> ItemsCopy = NearbyItems;
	for (ADroppedItemActor* Item : ItemsCopy)
	{
		LootItem(Item);
	}
}

void ULootComponent::AddNearbyItem(ADroppedItemActor* Item)
{
	NearbyItems.AddUnique(Item);
	OnLootRangeChanged.Broadcast(NearbyItems);
}

void ULootComponent::RemoveNearbyItem(ADroppedItemActor* Item)
{
	NearbyItems.Remove(Item);
	OnLootRangeChanged.Broadcast(NearbyItems);
}




