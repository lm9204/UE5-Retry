#include "WeaponFragment.h"
#include "WeaponDataAsset.h"
#include "Components/WeaponComponent.h"

void UWeaponFragment::OnFragmentActivated(AActor* Owner)
{
	if (!WeaponData || !Owner) return;

	if (UWeaponComponent* WC = Owner->FindComponentByClass<UWeaponComponent>())
	{
		WC->EquipWeapon(WeaponData);
	}
}

void UWeaponFragment::OnFragmentDeactivated(AActor* Owner)
{
	if (!Owner) return;

	if (UWeaponComponent* WC = Owner->FindComponentByClass<UWeaponComponent>())
	{
		WC->UnEquipWeapon();
	}
}

FText UWeaponFragment::GetFragmentDescription() const
{
	if (!WeaponData) return FText::GetEmpty();

	return FText::Format(
		FText::FromString("공격력: {0} | 탄창: {1}"),
		FText::AsNumber(WeaponData->Damage),
		FText::AsNumber(WeaponData->MagSize)
	);
}
