// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DamageTypes.h"
#include "CombatComponent.generated.h"


UCLASS( ClassGroup=(Combat), meta=(BlueprintSpawnableComponent) )
class RETRY_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();

	UFUNCTION( BlueprintCallable, Category = "Combat" )
	void Attack();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat" )
	float BaseDamage = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat" )
	float AttackRange = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat" )
	EDamageType DamageType = EDamageType::Bullet;

	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	void PerformLineTrace();
	void ApplyDamageToTarget(AActor* Target);

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Debug")
	bool bShowDebugTrace = true;
};
