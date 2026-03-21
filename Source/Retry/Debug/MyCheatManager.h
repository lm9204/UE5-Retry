// MyCheatManager.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "Components/InventoryComponent.h"
#include "Components/DamageTypes.h"
#include "MyCheatManager.generated.h"

UCLASS()
class RETRY_API UMyCheatManager : public UCheatManager
{
	GENERATED_BODY()
public:
	UFUNCTION(Exec)
	void DamageMe(float Amount);

	UFUNCTION(Exec)
	void HealMe(float Amount);
	
	UFUNCTION(Exec)
	void KillMe();

	UFUNCTION(Exec)
	void TestAttack();

	UFUNCTION(Exec)
	void GiveItem(FString TypeName);

	UFUNCTION(Exec)
	void GiveArmor(float Reduction);

	UFUNCTION(Exec)
	void EquipItem(FName ItemID);

	UFUNCTION(Exec)
	void UnEquipItem(ESlotType SlotType);

	UFUNCTION(Exec)
	void TestArmorDamage(float Amount);

private:
	UInventoryComponent* GetInventory();
};