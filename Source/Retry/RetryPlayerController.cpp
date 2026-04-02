// Copyright Epic Games, Inc. All Rights Reserved.


#include "RetryPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Retry.h"
#include "Debug/MyCheatManager.h"
#include "Widgets/Input/SVirtualJoystick.h"

ARetryPlayerController::ARetryPlayerController()
{
	CheatClass = UMyCheatManager::StaticClass();
}

void ARetryPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);


	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogRetry, Error, TEXT("Could not spawn mobile controls widget."));

		}
	}
	if (InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UInventoryWidget>(this, InventoryWidgetClass);
		InventoryWidget->AddToViewport();
		InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
		
	}

	if (LootWidgetClass)
	{
		LootWidget = CreateWidget<ULootWidget>(this, LootWidgetClass);
		LootWidget->AddToViewport();
		LootWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	
}

void ARetryPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		// Bind Click Move
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			if (ClickMoveAction)
			{
				EnhancedInputComponent->BindAction(ClickMoveAction, ETriggerEvent::Started,
					this, &ARetryPlayerController::OnClickMove);
			}

			if (IA_Inventory)
			{
				EnhancedInputComponent->BindAction(IA_Inventory, ETriggerEvent::Started, this,
					&ARetryPlayerController::ToggleInventory);
			}
		}
	}
}

void ARetryPlayerController::OnClickMove()
{
	FHitResult HitResult;
	GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	if (HitResult.bBlockingHit)
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, HitResult.Location);
	}
}

void ARetryPlayerController::ToggleInventory()
{
	if (!InventoryWidget) return;

	bool bVisible = InventoryWidget->IsVisible();
	UE_LOG(LogTemp, Warning, TEXT("ToggleInventory")); 

	if (bVisible)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		// 열 떄 데이터 갱신	
		APawn* P = GetPawn();
		if (P)
		{
			UInventoryComponent* Inv = P->FindComponentByClass<UInventoryComponent>();
			if (Inv)
			{
				InventoryWidget->RefreshInventory(
					Inv->GetAllItems(),
					Inv->GetEquippedSlots()
				);
			}
		}

		InventoryWidget->SetVisibility(ESlateVisibility::Visible);
	}
}
