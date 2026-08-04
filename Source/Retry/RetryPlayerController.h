// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/InventoryWidget.h"
#include "UI/LootWidget.h"
#include "GameFramework/PlayerController.h"
#include "RetryPlayerController.generated.h"

class UInputMappingContext;
class UScenarioDebugWidget;
class UUserWidget;
class UInputAction;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class ARetryPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARetryPlayerController();
	
protected:
	
	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditAnywhere, Category="Input|Actions")
	TObjectPtr<UInputAction> ClickMoveAction;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> PlayerHUDClass;
	
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<ULootWidget> LootWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI|Scenario")
	TSubclassOf<UScenarioDebugWidget> ScenarioDebugWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TObjectPtr<UInputAction> IA_Inventory;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_Fire;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_Reload;

	UPROPERTY(EditDefaultsOnly, Category="Input|Actions")
	TObjectPtr<UInputAction> IA_ScenarioDebug;
	
	UPROPERTY()
	UInventoryWidget* InventoryWidget;

	UPROPERTY()
	ULootWidget* LootWidget;

	UPROPERTY()
	UUserWidget* PlayerHUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<UScenarioDebugWidget> ScenarioDebugWidget;


	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;
private:
	void OnClickMove();
	void ToggleInventory();
	void ToggleScenarioDebug();

	// Weapon
	void OnFireStarted();
	void OnFireCompleted();
	void OnReload();
};
