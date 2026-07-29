// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"
#include "Actor/DroppedItemActor.h"
#include "Debug/CombatLogging.h"
#include "InventoryComponent.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::TakeDamage(FDamageInfo Info)
{
	if (bIsDead) return;

	float Final = CalculateFinalDamage(Info.BaseDamage);
	CurrentHealth = FMath::Clamp(CurrentHealth - Final, 0.f, MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	OnHitReaction.Broadcast(Info);

	UE_LOG(LogTemp, Warning, TEXT("[Health] %s TakeDamage: %.1f -> Final: %1.f -> HP: %.1f"),
		*GetCombatLogName(GetOwner()), Info.BaseDamage, Final, CurrentHealth);

	if (CurrentHealth <= 0.f)
		HandleDeath();
}

void UHealthComponent::Heal(float Amount)
{
	if (bIsDead) return;

	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentHealth = MaxHealth;
}

// @todo implement replicate for multiplay
// void UHealthComponent::OnRep_CurrentHealth()
// {
// 	
// }

void UHealthComponent::HandleDeath()
{
	bIsDead = true;

	// 인벤토리 아이템 드롭
	if (UInventoryComponent* Inv = GetOwner()->FindComponentByClass<UInventoryComponent>())
	{
		TArray<FItemInstance> Items = Inv->GetAllItems();
		for (const FItemInstance& Item : Items)
		{
			// 랜덤 위치에 드롭
			FVector DropLocation = GetOwner()->GetActorLocation()
				+ FVector(FMath::RandRange(-50.f, 50.f), FMath::RandRange(-50.f, 50.f), 0.f);

			ADroppedItemActor* Dropped = GetWorld()->SpawnActor<ADroppedItemActor>(
				ADroppedItemActor::StaticClass(),
				DropLocation,
				FRotator::ZeroRotator
			);

			if (Dropped) Dropped->ItemData = Item;
		}
	}
	
	OnDeath.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("[Health] %s is Dead!"), *GetCombatLogName(GetOwner()));
}

float UHealthComponent::CalculateFinalDamage(float BaseDamage) const
{
	float ArmorReduction = 0.f;

	if (AActor* Owner = GetOwner())
	{
		if (UInventoryComponent* Inv = GetOwner()->FindComponentByClass<UInventoryComponent>())
		{
			ArmorReduction = Inv->GetTotalArmorReduction();
		}
	}
	
	return BaseDamage * (1.f - ArmorReduction);
}

