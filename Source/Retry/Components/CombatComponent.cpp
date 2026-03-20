// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"

#include "HealthComponent.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::Attack()
{
	PerformLineTrace();
}

// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UCombatComponent::PerformLineTrace()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	FVector Start = Owner->GetActorLocation();
	FVector End = Start + Owner->GetActorForwardVector() * AttackRange;

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Pawn,
		Params
	);

	// Debug Line
	if (bShowDebugTrace)
	{
		FColor Color = bHit ? FColor::Red : FColor::Green;
		DrawDebugLine(GetWorld(), Start, End, Color, false, 1.f, 0, 2.f);
	}

	if (bHit && HitResult.GetActor())
	{
		ApplyDamageToTarget(HitResult.GetActor());
	}
}

void UCombatComponent::ApplyDamageToTarget(AActor* Target)
{
	UHealthComponent* HealthComponent = Target->FindComponentByClass<UHealthComponent>();
	if (!HealthComponent) return;

	FDamageInfo Info;
	Info.BaseDamage = BaseDamage;
	Info.DamageType = DamageType;
	Info.Instigator = GetOwner();

	HealthComponent->TakeDamage(Info);
}

