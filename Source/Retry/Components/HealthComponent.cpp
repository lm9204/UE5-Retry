// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"

#include "DroppedItemActor.h"
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

	UE_LOG(LogTemp, Warning, TEXT("[Health] TakeDamage: %.1f -> Final: %1.f -> HP: %.1f"),
		Info.BaseDamage, Final, CurrentHealth);

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
	UInventoryComponent* Inv = GetOwner()->FindComponentByClass<UInventoryComponent>();
	if (Inv)
	{
		TArray<FItemData> Items = Inv->GetAllItems();
		for (const FItemData& Item : Items)
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

	UE_LOG(LogTemp, Warning, TEXT("[Health] %s is Dead!"), *GetOwner()->GetName());
}

float UHealthComponent::CalculateFinalDamage(float BaseDamage) const
{
	UInventoryComponent* Inv = GetOwner()->FindComponentByClass<UInventoryComponent>();
	float Reduction = Inv ? Inv->GetTotalArmorReduction() : 0.f;
	return BaseDamage * (1.f - Reduction);
}

