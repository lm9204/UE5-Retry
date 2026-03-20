
#include "MyCheatManager.h"

#include "Components/CombatComponent.h"
#include "GameFramework/Character.h"
#include "Components/HealthComponent.h"

void UMyCheatManager::DamageMe(float Amount)
{
	ACharacter* P = Cast<ACharacter>(GetOuterAPlayerController()->GetPawn());
	if (!P) return;

	UHealthComponent* HealthComponent = P->FindComponentByClass<UHealthComponent>();
	if (!HealthComponent) return;

	FDamageInfo Info;
	Info.BaseDamage = Amount;
	HealthComponent->TakeDamage(Info);
}

void UMyCheatManager::HealMe(float Amount)
{
	ACharacter* P = Cast<ACharacter>(GetOuterAPlayerController()->GetPawn());
	if (!P) return;

	UHealthComponent* HealthComponent = P->FindComponentByClass<UHealthComponent>();
	if (!HealthComponent) return;

	HealthComponent->Heal(Amount);
}

void UMyCheatManager::KillMe()
{
	DamageMe(99999.f);
}

void UMyCheatManager::TestAttack()
{
	ACharacter* P = Cast<ACharacter>(GetOuterAPlayerController()->GetPawn());
	if (!P) return;

	UCombatComponent* CC = P->FindComponentByClass<UCombatComponent>();
	if (!CC) return;

	CC->Attack();
}
