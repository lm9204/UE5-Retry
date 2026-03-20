// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class RETRY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	UInventoryComponent();

	// ── 외부 인터페이스 ──────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(FItemData Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipItem(FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UnEquipItem(ESlotType Slot);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetTotalArmorReduction() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetTotalWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(FName ItemID) const;

	// ── Delegate ─────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	// ── 디자이너 설정값 ──────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory")
	float MaxWeight = 30.f;
	
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<FItemData> Items;

	UPROPERTY()
	TMap<ESlotType, FItemData> EquippedItems;


	FItemData* FindItem(FName ItemID);
	void ApplyWeightPenalty();
};
