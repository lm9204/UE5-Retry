#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "AmmoFragment.generated.h"

UCLASS(DisplayName="Ammo Fragment")
class RETRY_API UAmmoFragment : public UItemFragment
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ammo")
	int32 AmmoCount = 30;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ammo")
	FName CompatibleWeaponID;

	virtual void OnFragmentActivated(AActor* Owner) override;
	virtual FText GetFragmentDescription() const override;
};
