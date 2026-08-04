// Copyright Epic Games, Inc. All Rights Reserved.


#include "RetryPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Retry.h"
#include "Components/WeaponComponent.h"
#include "Debug/CombatLogging.h"
#include "Debug/MyCheatManager.h"
#include "UI/ScenarioDebugWidget.h"
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

	if (ScenarioDebugWidgetClass && IsLocalPlayerController())
	{
		ScenarioDebugWidget = CreateWidget<UScenarioDebugWidget>(
			this, ScenarioDebugWidgetClass);
		if (ScenarioDebugWidget)
		{
			ScenarioDebugWidget->AddToViewport(100);
			ScenarioDebugWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
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
				EnhancedInputComponent->BindAction(IA_Inventory, ETriggerEvent::Started,
					this, &ARetryPlayerController::ToggleInventory);
			}

			if (IA_Fire)
			{
				EnhancedInputComponent->BindAction(IA_Fire, ETriggerEvent::Started,
					this, &ARetryPlayerController::OnFireStarted);
				EnhancedInputComponent->BindAction(IA_Fire, ETriggerEvent::Completed,
					this, &ARetryPlayerController::OnFireCompleted);
			}

			if (IA_Reload)
			{
				EnhancedInputComponent->BindAction(IA_Reload, ETriggerEvent::Started,
					this, &ARetryPlayerController::OnReload);
			}

			if (IA_ScenarioDebug)
			{
				EnhancedInputComponent->BindAction(IA_ScenarioDebug, ETriggerEvent::Started,
					this, &ARetryPlayerController::ToggleScenarioDebug);
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
	UE_LOG(LogTemp, Warning, TEXT("%s ToggleInventory"), *GetCombatLogName(GetPawn()));

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

void ARetryPlayerController::ToggleScenarioDebug()
{
	if (!ScenarioDebugWidget) return;

	if (ScenarioDebugWidget->IsVisible())
	{
		ScenarioDebugWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ScenarioDebugWidget->RefreshRunContext();
	ScenarioDebugWidget->SetVisibility(ESlateVisibility::Visible);
}

void ARetryPlayerController::OnFireStarted()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	if (UWeaponComponent* WeaponComp = MyPawn->FindComponentByClass<UWeaponComponent>())
	{
		WeaponComp->Fire();
	}
}

void ARetryPlayerController::OnFireCompleted()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	if (UWeaponComponent* WeaponComp = MyPawn->FindComponentByClass<UWeaponComponent>())
	{
		WeaponComp->StopFire();
	}
}

void ARetryPlayerController::OnReload()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	if (UWeaponComponent* WeaponComp = MyPawn->FindComponentByClass<UWeaponComponent>())
	{
		WeaponComp->Reload();
	}
}
