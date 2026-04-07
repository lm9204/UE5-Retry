// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LootComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLootRangeChanged, const TArray<ADroppedItemActor*>&, NearbyItems);

UCLASS( ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent) )
class RETRY_API ULootComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULootComponent();

	// ── 외부 인터페이스 ──────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "Loot")
	bool LootItem(ADroppedItemActor* DroppedItem);

	UFUNCTION(BlueprintCallable, Category = "Loot")
	void LootAll();
	
	TArray<ADroppedItemActor*> GetNearbyItems() const { return NearbyItems; }

	// ── Delegate ─────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category = "Loot|Events")
	FOnLootRangeChanged OnLootRangeChanged;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<ADroppedItemActor*> NearbyItems;

	UPROPERTY()
	class UInventoryComponent* OwnerInventory;

	void AddNearbyItem(ADroppedItemActor* Item);
	void RemoveNearbyItem(ADroppedItemActor* Item);

	friend class ADroppedItemActor;
};
