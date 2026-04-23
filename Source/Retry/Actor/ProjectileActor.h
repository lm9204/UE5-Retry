#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileActor.generated.h"

UCLASS()
class RETRY_API AProjectileActor : public AActor
{
	GENERATED_BODY()
	
public:
	AProjectileActor();

	UPROPERTY(EditDefaultsOnly, Category="Projectile")
	float Damage = 25.f;

protected:
		virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category="Components")
	class USphereComponent* CollisionComponent;
	
	UPROPERTY(VisibleAnywhere, Category="Components")
	class UProjectileMovementComponent* ProjectileMovement;
	
	UPROPERTY(VisibleAnywhere, Category="Components")
	class UStaticMeshComponent* MeshComponent;

private:
	UFUNCTION()
	void onHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse,
		const FHitResult& Hit);
	
};
