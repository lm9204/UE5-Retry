#pragma once

#include "CoreMinimal.h"
#include "GroupMemoryTypes.generated.h"

USTRUCT(BlueprintType)
struct FGroupMemoryEvent
{
	GENERATED_BODY()

	// 이 사건을 직접 목격한 NPC (목격자만 이 이벤트를 인지함)
	FString WitnessID;

	FString EventType;      // "AllyDeath", "CombatStart", "TookHeavyDamage" 등
	FVector Location = FVector::ZeroVector;
	float Timestamp = 0.f;
	float EmotionWeight = 0.f;
	FString Description;
};