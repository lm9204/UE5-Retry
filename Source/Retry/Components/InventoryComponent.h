// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/ItemTypes.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class RETRY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	UInventoryComponent();

	// ── 외부 인터페이스 ──────────인터페이스────────────────────────
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(FItemInstance Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipItem(FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UnEquipItem(ESlotType Slot);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseAmmo(FName AmmoItemID);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetTotalAmmoCount(FName AmmoItemID);
	
	FItemInstance* FindItemByID(FName ItemID);
	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FItemInstance> GetAllItems() const { return Items; }
	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetTotalArmorReduction() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetTotalWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(FName ItemID) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TMap<ESlotType, FItemInstance> GetEquippedItems() const { return EquippedItems; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FEquippedItemSlot> GetEquippedSlots() const;

	void ActivateItemFragment(FName ItemID, AActor* Owner);
	void DeactivateItemFragment(FName ItemID, AActor* Owner);
	
	// ── Delegate ─────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	// ── 디자이너 설정값 ──────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory")
	float MaxWeight = 30.f;

	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	UDataTable* ItemDataTable;
	
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<FItemInstance> Items;

	UPROPERTY()
	TMap<ESlotType, FItemInstance> EquippedItems;
	
	void ApplyWeightPenalty();
	void RemoveItemInternal(FName ItemID);
};
