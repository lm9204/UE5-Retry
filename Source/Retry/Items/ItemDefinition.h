#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "Engine/DataAsset.h"
#include "WeaponFragment.h"
#include "ArmorFragment.h"
#include "AmmoFragment.h"
#include "ItemDefinition.generated.h"

UCLASS(BlueprintType)
class RETRY_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FName ItemID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	EItemType ItemType = EItemType::Misc;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FText Description;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	float Weight = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	class UTexture2D* Icon = nullptr;

	// Fragment 배열 - 인라인 편집 가능
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fragments", Instanced)
	TArray<class UItemFragment*> Fragments;
	
	UFUNCTION(BlueprintPure, Category="Item")
	UArmorFragment* GetArmorFragment() const { return FindFragment<UArmorFragment>(); }
	
	UFUNCTION(BlueprintPure, Category="Item")
	UWeaponFragment* GetWeaponFragment() const { return FindFragment<UWeaponFragment>(); }

	UFUNCTION(BlueprintPure, Category="Item")
	UAmmoFragment* GetAmmoFragment() const { return FindFragment<UAmmoFragment>(); }

	UFUNCTION(BlueprintPure, Category="Item")
	bool HasArmorFragment() const { return GetArmorFragment() != nullptr; }
	
	UFUNCTION(BlueprintPure, Category="Item")
	bool HasWeaponFragment() const { return GetWeaponFragment() != nullptr; }

	UFUNCTION(BlueprintPure, Category="Item")
	bool HasAmmoFragment() const { return GetAmmoFragment() != nullptr; }

	// 특정 Fragment 찾기
	template<typename T>
	T* FindFragment() const
	{
		for (UItemFragment* Fragment : Fragments)
		{
			if (T* Typed = Cast<T>(Fragment))
				return Typed;
		}
		return nullptr;
	}

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("ItemDefinition", ItemID);
	}

};