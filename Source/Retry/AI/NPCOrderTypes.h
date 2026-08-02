#pragma once

#include "CoreMinimal.h"
#include "NPCOrderTypes.generated.h"

UENUM(BlueprintType)
enum class ENPCOrder : uint8
{
	HoldFire   UMETA(DisplayName = "Hold Fire"),
	FreeFire   UMETA(DisplayName = "Free Fire"),
	Charge     UMETA(DisplayName = "Charge"),
	Retreat    UMETA(DisplayName = "Retreat"),
	TakeCover  UMETA(DisplayName = "Take Cover"),
};