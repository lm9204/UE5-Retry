#include "ProjectileActor.h"

#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileActor::AProjectileActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetSphereRadius(5.f);
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComponent->SetCollisionObjectType(ECC_GameTraceChannel2); // Projectile
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 5000.f;
	ProjectileMovement->MaxSpeed = 5000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.1f;

	InitialLifeSpan = 3.f;
}

void AProjectileActor::BeginPlay()
{
	Super::BeginPlay();

	// Existing Blueprint projectile defaults may retain the old WorldDynamic type.
	CollisionComponent->SetCollisionObjectType(ECC_GameTraceChannel2); // Projectile

	CollisionComponent->OnComponentHit.AddDynamic(this, &AProjectileActor::onHit);

	// 발사자 무시
	if (GetInstigator())
	{
		CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
	}
}

void AProjectileActor::onHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetInstigator()) return;

	//HealthComponent 있으면 피해 적용
	if (UHealthComponent* HealthComponent =
		OtherActor->FindComponentByClass<UHealthComponent>())
	{
		FDamageInfo DamageInfo;
		DamageInfo.BaseDamage = Damage;
		DamageInfo.DamageType = EDamageType::Bullet;
		DamageInfo.Instigator = GetInstigator();

		HealthComponent->TakeDamage(DamageInfo);

		UE_LOG(LogTemp, Warning, TEXT("[Projectile] 히트 %s -> %.1f 데미지"),
			*OtherActor->GetName(), Damage);
	}

	Destroy();
}
