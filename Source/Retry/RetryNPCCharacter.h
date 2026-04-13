// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "RetryNPCCharacter.generated.h"

UCLASS()
class RETRY_API ARetryNPCCharacter : public ACharacter,
	public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARetryNPCCharacter();

	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(1);
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UPersonalityComponent* PersonalityComponent;

	// NPC 전용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UAIPerceptionComponent* PerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UWidgetComponent* NameplateWidget;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<class UFloatingNameWidget> FloatingNameWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	FString NPCName = TEXT("NPC");

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	UFUNCTION()
	void OnDeath();

	UFUNCTION()
	void OnHealthChanged(float CurrentHP, float MaxHP);

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

};
