// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageTypes.generated.h"

/**
 * 
 */

UENUM(BlueprintType, meta=(ScriptName="RetryDamageType"))
enum class EDamageType : uint8 
{
	Bullet		UMETA(DisplayName = "Bullet"),
	Explosion	UMETA(DisplayName = "Explosion"),
	Melee		UMETA(DisplayName = "Melee"),
};

USTRUCT(BlueprintType)
struct FDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDamageType DamageType = EDamageType::Bullet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* Instigator = nullptr;
};
