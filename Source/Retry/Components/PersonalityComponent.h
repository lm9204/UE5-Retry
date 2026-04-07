// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PersonalityComponent.generated.h"

UENUM(BlueprintType)
enum class EPersonalityType : uint8
{
	Aggressive		UMETA(DisplayName = "Aggressive"),
	Cautious		UMETA(DisplayName = "Cautious"),
	Supportive		UMETA(DisplayName = "Supportive"),
	Opportunist		UMETA(DisplayName = "Opportunist"),
};

UCLASS( ClassGroup=(AI), meta=(BlueprintSpawnableComponent) )
class RETRY_API UPersonalityComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPersonalityComponent();

	UFUNCTION(BlueprintPure, Category = "Personality")
	EPersonalityType GetPersonalityType() const;

	UFUNCTION(BlueprintPure, Category = "Personality")
	float GetAggression() const { return Aggression; }

	UFUNCTION(BlueprintPure, Category = "Personality")
	float GetFear() const { return Fear; }

	UFUNCTION(BlueprintPure, Category = "Personality")
	float GetTrust() const { return Trust; }

	// Blackboard 즉시 업데이트
	void SyncToBlackboard();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Personality")
	EPersonalityType DefaultPersonality = EPersonalityType::Opportunist;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Personality")
	float Aggression = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Personality")
	float Fear = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Personality")
	float Trust = 0.5f;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	void ApplyPersonalityPreset();
	
};
