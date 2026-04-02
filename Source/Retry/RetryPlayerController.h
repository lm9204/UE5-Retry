// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InventoryWidget.h"
#include "LootWidget.h"
#include "GameFramework/PlayerController.h"
#include "RetryPlayerController.generated.h"

class UInputMappingContext;
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
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<ULootWidget> LootWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category="UI")
	UInputAction* IA_Inventory;
	
	UPROPERTY()
	UInventoryWidget* InventoryWidget;

	UPROPERTY()
	ULootWidget* LootWidget;


	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;
private:
	void OnClickMove();
	void ToggleInventory();
};
