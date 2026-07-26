// MyCheatManager.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "Components/InventoryComponent.h"
#include "Components/DamageTypes.h"
#include "RetryNPCCharacter.h"
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

	UFUNCTION(BlueprintCallable, Category="Cheat")
	void GiveItemToActor(AActor* TargetActor, FString TypeName, int32 Count = 1);
	
	UFUNCTION(Exec)
	void EquipItem(FName ItemID);

	UFUNCTION(Exec)
	void UnEquipItem(ESlotType SlotType);

	UFUNCTION(Exec)
	void TestArmorDamage(float Amount);

	UFUNCTION(Exec)
	void SetNPCAggression(float Value);
	
	UFUNCTION(Exec)
	void SpawnNPC(FString PersonalityType);

	UFUNCTION(Exec)
	void ToggleAIDebug();

	UPROPERTY(EditDefaultsOnly, Category="Cheat")
	TSubclassOf<ARetryNPCCharacter> NPCCharacterClass;

	UPROPERTY(EditDefaultsOnly, Category="Cheat")
	TMap<FString, UItemDefinition*> ItemDefinitionMap;
private:
	UInventoryComponent* GetInventory();
};