#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FloatingNameWidget.generated.h"

UCLASS()
class RETRY_API UFloatingNameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(blueprintNativeEvent, Category="UI")
	void SetNameText(const FString& Name);

	UFUNCTION(blueprintNativeEvent, Category="UI")
	void SetHPPercent(float Percent); // 0.0 ~ 1.0
};