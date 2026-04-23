// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void RefreshInventory(const TArray<FItemInstance>& Items,
						  const TArray<FEquippedItemSlot>& EquippedItems);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void ShowToolTip(FItemInstance Item, FVector2D ScreenPosition);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void HideToolTip();

protected:

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RequestEquip(FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RequestUnEquip(ESlotType SlotType);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RequestDrop(FName ItemID);

private:
	UFUNCTION()
	void OnInventoryChanged();
	
	UPROPERTY()
	class UInventoryComponent* OwnerInventory;

	UPROPERTY()
	class ULootComponent* OwnerLoot;

	UPROPERTY()
	UUserWidget* InventoryTooltipWidget;

	virtual void NativeConstruct() override;
	
};
