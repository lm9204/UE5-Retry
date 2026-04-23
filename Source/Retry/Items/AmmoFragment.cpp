#include "AmmoFragment.h"

#include "Components/WeaponComponent.h"

void UAmmoFragment::OnFragmentActivated(AActor* Owner)
{
	if (!Owner) return;

	if (UWeaponComponent* WC = Owner->FindComponentByClass<UWeaponComponent>())
	{
		WC->AddReserveAmmo(AmmoCount);
	}
}

FText UAmmoFragment::GetFragmentDescription() const
{
	return FText::Format(
		FText::FromString("탄약: {0}발"),
		FText::AsNumber(AmmoCount)
	);
}
