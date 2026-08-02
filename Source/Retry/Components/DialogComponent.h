#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DialogComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueRequested,
	const FString&, Text, float, Duration);

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RETRY_API UDialogComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDialogComponent();

	UFUNCTION(BlueprintCallable, Category="Dialogue")
	void ShowDialogue(const FString& Text, float Duration = 4.f);

	UPROPERTY(BlueprintAssignable, Category="Dialogue")
	FOnDialogueRequested OnDialogueRequested;

private:
	FTimerHandle DialogueTimerHandle;
};