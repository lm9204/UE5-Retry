// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/WeaponComponent.h"

#include "Actor/ProjectileActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values for this component's properties
UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponComponent::AddReserveAmmo(int32 Amount)
{
	ReserveAmmo = FMath::Clamp(ReserveAmmo + Amount, 0, MaxReserveAmmo);
	UE_LOG(LogTemp, Warning, TEXT("[Weapon] 탄약 보충 +%d → 예비: %d"),
		Amount, ReserveAmmo);
}

void UWeaponComponent::EquipWeapon(UWeaponDataAsset* WeaponData)
{
	if (!WeaponData) return;

	CurrentWeaponData = WeaponData;
	CurrentAmmo = 0;
	bIsReloading = false;

	//무기 메시 붙이기
	if (WeaponData->WeaponMesh)
	{
		if (!WeaponMeshComp)
		{
			WeaponMeshComp = NewObject<USkeletalMeshComponent>(GetOwner());
			WeaponMeshComp->RegisterComponent();
		}

		WeaponMeshComp->SetSkeletalMesh(WeaponData->WeaponMesh);

		//오른손 소켓에 부착
		if (USkeletalMeshComponent* OwnerMesh = GetOwnerMesh())
		{
			WeaponMeshComp->AttachToComponent(OwnerMesh,
				FAttachmentTransformRules::SnapToTargetIncludingScale,
				TEXT("HandGrip_R"));

			// 오프셋 적용
			WeaponMeshComp->SetRelativeLocation(
				WeaponData->GripLocationOffset);
			WeaponMeshComp->SetRelativeRotation(
				WeaponData->GripRotationOffset);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Weapon] 장착 %s"), *WeaponData->WeaponName);
}

void UWeaponComponent::UnEquipWeapon()
{
	if (WeaponMeshComp)
	{
		WeaponMeshComp->DetachFromComponent(
			FDetachmentTransformRules::KeepWorldTransform);
		WeaponMeshComp->SetVisibility(false);
	}

	CurrentWeaponData = nullptr;
	StopFire();
}

void UWeaponComponent::Fire()
{
	if (!bIsAiming) return;
	if (!CurrentWeaponData || bIsReloading) return;
	
	if (CurrentWeaponData->FireMode == EFireMode::SemiAuto)
	{
		FireOnce();
	}
	else
	{
		StartFire();
	}
}

void UWeaponComponent::StartFire()
{
	if (!CurrentWeaponData || bIsReloading || bIsFiring) return;

	bIsFiring = true;
	FireOnce();

	GetWorld()->GetTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&UWeaponComponent::FireOnce,
		CurrentWeaponData->FireRate,
		true
	);
}

void UWeaponComponent::StopFire()
{
	bIsFiring = false;
	GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
}

void UWeaponComponent::StartAim()
{
	bIsAiming = true;
}

void UWeaponComponent::StopAim()
{
	bIsAiming = false;
}

void UWeaponComponent::Reload()
{
	if (!CurrentWeaponData || bIsReloading) return;
	if (CurrentAmmo == CurrentWeaponData->MagSize) return;
	if (ReserveAmmo <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Weapon] 탄약 없음 재장전 미완료."));
		return;
	}

	bIsReloading = true;
	StopFire();

	//재장전 애니메이션
	if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
	{
		if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
		{
			if (CurrentWeaponData->ReloadMontage)
			{
				float ReloadTime = AnimInst->Montage_Play(
					CurrentWeaponData->ReloadMontage);

				GetWorld()->GetTimerManager().SetTimer(
					ReloadTimerHandle,
					this,
					&UWeaponComponent::FinishReload,
					ReloadTime > 0.f ? ReloadTime : 2.f,
					false
				);
				return;
			}
		}
	}

	//애니메이션 없으면 2초후 완료
	GetWorld()->GetTimerManager().SetTimer(
		ReloadTimerHandle, this, &UWeaponComponent::FinishReload, 2.f, false);
}

void UWeaponComponent::FireOnce()
{
	if (!CurrentWeaponData) return;

	UE_LOG(LogTemp, Warning, TEXT("[Weapon:FireOnce] 탄약상태:%d."), CurrentAmmo);
	if (CurrentAmmo <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Weapon:FireOnce] 탄약없음."));
		StopFire();
		return;
	}

	CurrentAmmo--;
	SpawnProjectile();

	//발사 애니메이션
	if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
	{
		if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
		{
			if (CurrentWeaponData->FireMontage)
			{
				AnimInst->Montage_Play(CurrentWeaponData->FireMontage);
			}
		}
	}
	
	OnWeaponFired.Broadcast(CurrentAmmo, CurrentWeaponData->MagSize, ReserveAmmo);
}

void UWeaponComponent::SpawnProjectile()
{
	if (!CurrentWeaponData || !CurrentWeaponData->ProjectileClass) return;

	USkeletalMeshComponent* OwnerMesh = GetOwnerMesh();
	if (!OwnerMesh) return;


	// 총구 소켓 위치/방향
	FVector MuzzleLocation = OwnerMesh->GetSocketLocation(
		CurrentWeaponData->MuzzleSocketName);
	
	FVector Target = CachedAimTarget;
	Target.Z = MuzzleLocation.Z;

	FVector Direction = (Target - MuzzleLocation).GetSafeNormal();
	FRotator SpawnRotation = Direction.Rotation();

#if WITH_EDITOR
	DrawDebugLine(
	GetWorld(),
	MuzzleLocation,
	MuzzleLocation + Direction * 1000.f,
	FColor::Green,
	false,
	3.f,
	0,
	2.f   // 선 두께
);
	DrawDebugSphere(
		GetWorld(),
		CachedAimTarget,
		20.f,
		12,
		FColor::Blue,
		false,
		3.f
	);
#endif

	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.bDeferConstruction = true;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectileActor* Projectile = GetWorld()->SpawnActor<AProjectileActor>(
		CurrentWeaponData->ProjectileClass,
		MuzzleLocation, SpawnRotation, SpawnParams);

	if (IsValid(Projectile))
	{
		Projectile->Damage = CurrentWeaponData->Damage;

		if (UProjectileMovementComponent* ProjMove =
			Projectile->FindComponentByClass<UProjectileMovementComponent>())
		{
			ProjMove->InitialSpeed = CurrentWeaponData->ProjectileSpeed;
			ProjMove->MaxSpeed = CurrentWeaponData->ProjectileSpeed;
		}

		Projectile->FinishSpawning(FTransform(
			SpawnRotation, MuzzleLocation));
	}
}

void UWeaponComponent::FinishReload()
{
	if (!CurrentWeaponData) return;

	// 재장전 필요한 탄약 수 계싼
	int32 AmmoNeeded = CurrentWeaponData->MagSize - CurrentAmmo;
	int32 AmmoToLoad = FMath::Min(AmmoNeeded, ReserveAmmo);

	CurrentAmmo += AmmoToLoad;
	ReserveAmmo -= AmmoToLoad;
	
	bIsReloading = false;
	OnWeaponReloaded.Broadcast();
	
	UE_LOG(LogTemp, Warning, TEXT("[Weapon] 재장전 완료 %d -> %d|남은탄약: %d"),
		CurrentAmmo - AmmoToLoad, CurrentAmmo, ReserveAmmo);
}


USkeletalMeshComponent* UWeaponComponent::GetOwnerMesh() const
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	return OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
}

EWeaponType UWeaponComponent::GetWeaponType() const
{
	if (!CurrentWeaponData) return EWeaponType::None;
	return CurrentWeaponData->WeaponType;
}

void UWeaponComponent::SetAimTarget(FVector Target)
{
	CachedAimTarget = Target;
}
