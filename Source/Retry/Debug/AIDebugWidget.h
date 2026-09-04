#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Debug/AIDebugTypes.h"
#include "AIDebugWidget.generated.h"

UCLASS()
class RETRY_API UAIDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category="Debug")
	void UpdateDebugInfo(
		const FString& StateName,
		float AttackScore,
		float CoverScore,
		float RetreatScore,
		float ReloadScore,
		float HPRatio,
		int32 CurrentAmmo,
		const FString& TargetName,
		float DistToTarget
	);

	/** Kept separate so existing WBP_AIDebug combat event nodes remain valid. */
	UFUNCTION(BlueprintImplementableEvent, Category="Debug",
		meta=(DisplayName="Update Mission Debug Info"))
	void UpdateMissionDebugInfo(const FAIMissionDebugSnapshot& Snapshot);
};
