// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "Components/WidgetComponent.h"
#include "RetryNPCCharacter.generated.h"

UCLASS()
class RETRY_API ARetryNPCCharacter : public ACharacter,
	public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARetryNPCCharacter();

	UPROPERTY(EditAnywhere, Category="Team")
	uint8 TeamID = 2;

	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(TeamID);
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UPersonalityComponent* PersonalityComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	class UWeaponComponent* WeaponComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UWidgetComponent* NameplateWidget;

	UPROPERTY(EditInstanceOnly, Category="Patrol")
	TArray<AActor*> PatrolPoints;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<class UFloatingNameWidget> FloatingNameWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	FString NPCName = TEXT("NPC");

	UPROPERTY(VisibleAnywhere, Category="Debug")
	class UWidgetComponent* AIDebugWidget;

	UPROPERTY(EditDefaultsOnly, Category="Debug")
	TSubclassOf<class UAIDebugWidget> AIDebugWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	FString DefaultWeapon;

	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	FString DefaultAmmo;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	UFUNCTION()
	void OnDeath();

	UFUNCTION()
	void OnHealthChanged(float CurrentHP, float MaxHP);

};
