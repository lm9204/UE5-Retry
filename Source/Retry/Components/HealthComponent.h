// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DamageTypes.h"
#include "HealthComponent.generated.h"

// 델리게이트 타입선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, CurrentHealth, float, MaxHealth);

UCLASS( ClassGroup=(Combat), meta=(BlueprintSpawnableComponent) )
class RETRY_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UHealthComponent();

	UFUNCTION(BlueprintCallable, Category = "Health")
	void TakeDamage(FDamageInfo Info);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }
	
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bIsDead; }
	
	// ── Delegate ─────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHealthChanged OnHealthChanged;

	// ── Settings ─────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly)
	float MaxHealth = 100.f;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// 리플리케이션 콜백
	// UFUNCTION()
	// void OnRep_CurrentHealth();

private:
	// 런타임 상태 변수
	// UPROPERTY(ReplicatedUsing=OnRep_CurrentHealth)
	UPROPERTY()
	float CurrentHealth;

	bool bIsDead = false;
	
	void HandleDeath();
	float CalculateFinalDamage(float BaseDamage) const;
};
