// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "Components/WidgetComponent.h"
#include "Items/ItemDefinition.h"
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	class UMemoryComponent* MemoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UWidgetComponent* NameplateWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	class UDialogComponent* DialogComponent;

	UPROPERTY(VisibleAnywhere, Category="Dialogue")
	class UWidgetComponent* DialogueWidgetComponent;

	UPROPERTY(EditDefaultsOnly, Category="Dialogue")
	TSubclassOf<class UDialogueWidget> DialogueWidgetClass;

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

	/** Hit Reaction */
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	class UAnimMontage* HitMontage;

	UPROPERTY(EditInstanceOnly, Category="Group")
	class AGroupManagerActor* MyGroup;

	UPROPERTY(EditInstanceOnly, Category="Group")
	bool bIsGroupLeader = false;

	UPROPERTY(EditInstanceOnly, Category="Inventory")
	TArray<TObjectPtr<UItemDefinition>> StartingItems;

	UPROPERTY()
	bool bIsStaggered = false;

	UPROPERTY(EditAnywhere, Category="AI")
	bool bIsHighIntelligence = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:	
	UFUNCTION()
	void OnDeath();

	UFUNCTION()
	void OnHealthChanged(float CurrentHP, float MaxHP);

	/** Hit Reaction */
	UFUNCTION()
	void PlayHitReaction(FDamageInfo Info);

	UFUNCTION()
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnMemoryThresholdReached();

	UFUNCTION()
	void OnDialogueRequested(const FString& Text, float Duration);

};
