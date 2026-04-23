#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponDataAsset.generated.h"

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	SemiAuto	UMETA(DisplayName = "Semi-Auto"),
	FullAuto	UMETA(DisplayName = "Full-Auto"),
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None	UMETA(DisplayName="None"),
	Rifle	UMETA(DisplayName="Rifle"),
	Pistol	UMETA(DisplayName="Pistol"),
	SMG		UMETA(DisplayName="SMG"),
	Shotgun	UMETA(DisplayName="Shotgun"),
};

UCLASS()
class RETRY_API UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FString WeaponName = TEXT("Rifle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	EFireMode FireMode = EFireMode::SemiAuto;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	EWeaponType WeaponType = EWeaponType::Rifle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	float FireRate = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	int32 MagSize = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	int32 InitialReserveAmmo = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	int32 MaxReserveAmmo = 90;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	float Damage = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	float HeadShotMultiplier = 2.f;

	// 총기를 손에 붙일 때 적용할 오프셋
	UPROPERTY(EditDefaultsOnly, Category="Socket")
	FVector GripLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category="Socket")
	FRotator GripRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category="Projectile")
	TSubclassOf<class AProjectileActor> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile")
	float ProjectileSpeed = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category="Socket")
	FName MuzzleSocketName = TEXT("MuzzleSocket");

	UPROPERTY(EditDefaultsOnly, Category="Animation")
	class UAnimMontage* FireMontage;

	UPROPERTY(EditDefaultsOnly, Category="Animation")
	class UAnimMontage* ReloadMontage;

	UPROPERTY(EditDefaultsOnly, Category="Mesh")
	class USkeletalMesh* WeaponMesh;
};