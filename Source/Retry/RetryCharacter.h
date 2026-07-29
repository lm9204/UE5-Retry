// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Components/HealthComponent.h"
#include "Components/CombatComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/LootComponent.h"
#include "Components/WeaponComponent.h"
#include "Navigation/CrowdAgentInterface.h"
#include "RetryCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class ARetryCharacter : public ACharacter,
	public ICrowdAgentInterface,
	public IGenericTeamAgentInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(0);
	}
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

public:

	/** Constructor */
	ARetryCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	ULootComponent* LootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UWeaponComponent* WeaponComponent;

	/** 적을 조준할 때 캡슐 중심으로부터 위로 얼마나 올려서 조준할지 (캡슐 HalfHeight 대비 비율) */
	UPROPERTY(EditAnywhere, Category="Combat")
	float EnemyAimHeightRatio = 0.6f;

	/** 바닥/벽 등 적이 아닌 곳을 조준할 때의 고정 높이 오프셋 */
	UPROPERTY(EditAnywhere, Category="Combat")
	float GroundAimHeightOffset = 60.f;

	/** Hit Reaction */
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	class UAnimMontage* HitMontage;

	UPROPERTY()
	bool bIsStaggered = false;

	/** Health Bar */
	UPROPERTY(VisibleAnywhere, Category="UI")
	class UWidgetComponent* HealthBarWidget;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<class UFloatingNameWidget> HealthBarWidgetClass;

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Hit Reaction */
	UFUNCTION()
	void PlayHitReaction(FDamageInfo Info);

	UFUNCTION()
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	virtual FVector GetCrowdAgentLocation() const override;
	virtual FVector GetCrowdAgentVelocity() const override;
	virtual void GetCrowdAgentCollisions(float& CylinderRadius,
		float& CylinderHalfHeight) const override;
	virtual float GetCrowdAgentMaxSpeed() const override;

private:
	UFUNCTION()
	void OnDeath();

	UFUNCTION()
	void OnPlayerHealthChanged(float CurrentHP, float MaxHP);

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

