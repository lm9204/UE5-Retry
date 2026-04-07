#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ItemTypes.h"
#include "Components/LootComponent.h"
#include "LootWidget.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API ULootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void RefreshLootList(const TArray<ADroppedItemActor*>& NearbyItems);

protected:
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void RequestLoot(ADroppedItemActor* Item);

	UFUNCTION(BlueprintCallable, Category = "Loot")
	void RequestLootAll();

private:
	UPROPERTY()
	ULootComponent* OwnerLoot;

	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnLootRangeChanged(const TArray<ADroppedItemActor*>& NearbyItems);
};
