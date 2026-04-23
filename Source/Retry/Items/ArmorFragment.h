#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "Items/ItemTypes.h"
#include "ArmorFragment.generated.h"

UCLASS(DisplayName="Armor Fragment")
class RETRY_API UArmorFragment : public UItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor")
	float ArmorReduction = 0.2f; // 0.0 ~ 0.8f
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor")
	ESlotType SlotType = ESlotType::Body;

	virtual FText GetFragmentDescription() const override;
};
