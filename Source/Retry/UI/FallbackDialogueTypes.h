#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Components/PersonalityComponent.h"
#include "FallbackDialogueTypes.generated.h"

USTRUCT(BlueprintType)
struct FFallbackDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Dialogue")
	EPersonalityType PersonalityType = EPersonalityType::Opportunist;

	UPROPERTY(EditAnywhere, Category="Dialogue")
	FString DialogueText;
};