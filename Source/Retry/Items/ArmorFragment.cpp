#include "ArmorFragment.h"


FText UArmorFragment::GetFragmentDescription() const
{
	return FText::Format(
		FText::FromString("방어율: {0}%"),
		FText::AsNumber(FMath::RoundToInt(ArmorReduction * 100))
	);
}
