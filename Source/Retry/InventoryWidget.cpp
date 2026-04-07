// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryWidget.h"

#include "DroppedItemActor.h"
#include "RetryPlayerController.h"
#include "Components/InventoryComponent.h"
#include "Components/LootComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ACharacter* Player = Cast<ACharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Player) return;

	OwnerInventory = Player->FindComponentByClass<UInventoryComponent>();
	OwnerLoot = Player->FindComponentByClass<ULootComponent>();

	// 인벤토리 변경 시 자동 갱신
	if (OwnerInventory)
	{
		OwnerInventory->OnInventoryChanged.AddDynamic(
			this, &UInventoryWidget::OnInventoryChanged);
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] OnInventoryChanged 바인딩 완료"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[InventoryWidget] InventoryComponent 없음"));
	}
}

void UInventoryWidget::OnInventoryChanged()
{
	UE_LOG(LogTemp, Warning, TEXT("OnInventoryChanged 호출"));
	
	if (OwnerInventory)
	{
		RefreshInventory(
			OwnerInventory->GetAllItems(),
			OwnerInventory->GetEquippedSlots()
		);
	}
}

void UInventoryWidget::RequestEquip(FName ItemID)
{
	if (OwnerInventory) OwnerInventory->EquipItem(ItemID);
}

void UInventoryWidget::RequestUnEquip(ESlotType SlotType)
{
	if (OwnerInventory) OwnerInventory->UnEquipItem(SlotType);
}

void UInventoryWidget::RequestDrop(FName ItemID)
{
	if (!OwnerInventory) return;

	// 인벤토리에서 제거
	FItemData* Item = OwnerInventory->GetItemData(ItemID);
	if (!Item) return;

	FVector DropLocation = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)
		->GetActorLocation() + FVector(100.f, 0.f,0.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.bDeferConstruction = true;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	ADroppedItemActor* Dropped = GetWorld()->SpawnActor<ADroppedItemActor>(
		ADroppedItemActor::StaticClass(), DropLocation, FRotator::ZeroRotator, SpawnParams);

	if (Dropped)
	{
		Dropped->ItemData = *Item;
		OwnerInventory->RemoveItem(ItemID);
		Dropped->FinishSpawning(FTransform(FRotator::ZeroRotator, DropLocation));
	}
}

