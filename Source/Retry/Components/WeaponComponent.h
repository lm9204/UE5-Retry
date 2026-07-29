// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/WeaponDataAsset.h"
#include "WeaponComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponFired, int32, CurrentAmmo, int32, MaxAmmo, int32, ReserveAmmo);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponReloaded);

UCLASS( ClassGroup=(Combat), meta=(BlueprintSpawnableComponent) )
class RETRY_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UWeaponComponent();
	void AddReserveAmmo(int32 Amount);
	void SetAimTarget(FVector Target);
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void EquipWeapon(UWeaponDataAsset* WeaponData);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void UnEquipWeapon();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Fire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartAim();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StopAim();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Reload();

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool IsArmed() const { return CurrentWeaponData != nullptr; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool GetIsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool GetIsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	EWeaponType GetWeaponType() const;

	UFUNCTION(BlueprintPure, Category="Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	int32 GetReserveAmmo() const { return ReserveAmmo; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	int32 GetMagSize() const { return CurrentWeaponData ? CurrentWeaponData->MagSize : 0; }

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FOnWeaponFired OnWeaponFired;
	
	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FOnWeaponReloaded OnWeaponReloaded;

private:
	UPROPERTY()
	UWeaponDataAsset* CurrentWeaponData = nullptr;

	UPROPERTY()
	class USkeletalMeshComponent* WeaponMeshComp = nullptr;

	FVector CachedAimTarget = FVector::ZeroVector;
	float LastFireTime = 0.f;
	int32 CurrentAmmo = 0;
	int32 ReserveAmmo = 0; //예비 탄약
	int32 MaxReserveAmmo = 90; //최대 예비 탄약
	bool bIsReloading = false;
	bool bIsFiring = false;
	bool bIsAiming = false;

	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;

	void FireOnce();
	void SpawnProjectile();
	void FinishReload();

	USkeletalMeshComponent* GetOwnerMesh() const;
};
