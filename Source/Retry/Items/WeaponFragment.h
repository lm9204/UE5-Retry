#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "WeaponFragment.generated.h"

UCLASS(DisplayName="Weapon Fragment")
class RETRY_API UWeaponFragment : public UItemFragment
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	class UWeaponDataAsset* WeaponData = nullptr;

	virtual void OnFragmentActivated(AActor* Owner) override;
	virtual void OnFragmentDeactivated(AActor* Owner) override;
	virtual FText GetFragmentDescription() const override;
};
