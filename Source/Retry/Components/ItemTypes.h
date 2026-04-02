#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Weapon		UMETA(DisplayName="Weapon"),
	Armor		UMETA(DisplayName="Armor"),
	Consumable	UMETA(DisplayName="Consumable"),
	Misc		UMETA(DisplayName="Misc"),
};

UENUM(BlueprintType)
enum class ESlotType : uint8
{
	None	UMETA(DisplayName="None"),
	Head	UMETA(DisplayName="Head"),
	Body	UMETA(DisplayName="Body"),
	Legs	UMETA(DisplayName="Legs"),
	Weapon	UMETA(DisplayName="Weapon"),
};

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemType ItemType = EItemType::Misc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESlotType SlotType = ESlotType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseDamage = 0.f; // Weapon 전용

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ArmorReduction = 0.f; // Armor 전용 (0.0 ~ 0.8)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StackCount = -1; // Consumable 전용
};

USTRUCT(BlueprintType)
struct FEquippedItemSlot
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESlotType Slot = ESlotType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FItemData Item;
};