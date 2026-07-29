// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetryCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "FloatingNameWidget.h"
#include "InputActionValue.h"
#include "Retry.h"
#include "RetryNPCCharacter.h"
#include "Debug/CombatLogging.h"
#include "Components/WidgetComponent.h"
#include "Navigation/CrowdManager.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"


ARetryCharacter::ARetryCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Create a Components
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	LootComponent = CreateDefaultSubobject<ULootComponent>(TEXT("LootComponent"));
	WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));

	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidget->SetupAttachment(RootComponent);
	HealthBarWidget->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidget->SetDrawSize(FVector2D(150.f, 40.f));
}

void ARetryCharacter::BeginPlay()
{
	Super::BeginPlay();

	SetActorRotation(FRotator(0.f, 45.f, 0.f));
	if (HealthBarWidgetClass)
	{
		HealthBarWidget->SetWidgetClass(HealthBarWidgetClass);

		if (UFloatingNameWidget* Widget =
			Cast<UFloatingNameWidget>(HealthBarWidget->GetUserWidgetObject()))
		{
			Widget->SetNameText(TEXT("Player"));  // 이름 비워도 됨
			Widget->SetHPPercent(1.f);      // 이거 호출 안 하면 HPBar가 초기 이상값일 수 있음
		}
		
		UE_LOG(LogTemp, Warning, TEXT("[Player] %s HealthBarWidget 클래스 설정: %s"),
			*GetCombatLogName(this), *HealthBarWidgetClass->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Player] %s HealthBarWidgetClass NULL"), *GetCombatLogName(this));
	}
	
	HealthComponent->OnDeath.AddDynamic(this, &ARetryCharacter::OnDeath);
	HealthComponent->OnHealthChanged.AddDynamic(this, &ARetryCharacter::OnPlayerHealthChanged);
	HealthComponent->OnHitReaction.AddDynamic(this, &ARetryCharacter::PlayHitReaction);

	// HitReaction 설정
	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		AnimInst->OnMontageEnded.AddDynamic(this, &ARetryCharacter::OnHitMontageEnded);
	}
	
	if (UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(GetWorld()))
	{
		CrowdManager->RegisterAgent(this);
	}

	UAIPerceptionStimuliSourceComponent* StimuliSource =
		FindComponentByClass<UAIPerceptionStimuliSourceComponent>();

	if (!StimuliSource)
	{
		StimuliSource = NewObject<UAIPerceptionStimuliSourceComponent>(this);
		StimuliSource->RegisterComponent();
	}

	StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
	StimuliSource->RegisterWithPerceptionSystem();
}

void ARetryCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	AActor* O = GetOwner();
	if (!O) return;

	if (PC->IsInputKeyDown(EKeys::RightMouseButton))
	{
		// 우클릭 시 마우스 바라보기
		// ECC_Visibility는 Pawn(캡슐) 콜리전 프로필이 기본적으로 Ignore 처리하기 때문에
		// 적을 그냥 통과해서 바닥에 꽂힌다. 적을 우선적으로 잡아내려면 ECC_Pawn으로 트레이스해야 한다.
		FHitResult HitResult;
		PC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult);

		if (HitResult.bBlockingHit)
		{
			FVector AimPoint = HitResult.Location + FVector(0, 0, GroundAimHeightOffset);

			if (ARetryNPCCharacter* HitNPC = Cast<ARetryNPCCharacter>(HitResult.GetActor()))
			{
				// 적을 맞췄다면 클릭한 지점이 아니라 적의 상체/머리 라인으로 조준점을 스냅
				const float HalfHeight = HitNPC->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				AimPoint = HitNPC->GetActorLocation() + FVector(0, 0, HalfHeight * EnemyAimHeightRatio);
			}

			FVector Direction = AimPoint - GetActorLocation();
			Direction.Z = 0.f;

			if (!Direction.IsNearlyZero())
			{
				SetActorRotation(Direction.Rotation());
				WeaponComponent->SetAimTarget(AimPoint);
			}
		}

		WeaponComponent->StartAim();
	}
	else
	{
		WeaponComponent->StopAim();
		// WC->SetAimTarget(FVector::ZeroVector);
	}
}

void ARetryCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARetryCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ARetryCharacter::Look);

		// Looking
		// EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARetryCharacter::Look);
	}
	else
	{
		UE_LOG(LogRetry, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetCombatLogName(this));
	}
}

void ARetryCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ARetryCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ARetryCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		FRotator FixedRotation = FRotator(0.f, 0.f, 0.f);
		
		const FVector ForwardDirection = FRotationMatrix(FixedRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection   = FRotationMatrix(FixedRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ARetryCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ARetryCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ARetryCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

FVector ARetryCharacter::GetCrowdAgentLocation() const
{
	return GetActorLocation();
}

FVector ARetryCharacter::GetCrowdAgentVelocity() const
{
	return GetVelocity();
}

void ARetryCharacter::GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const
{
	CylinderRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	CylinderHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

float ARetryCharacter::GetCrowdAgentMaxSpeed() const
{
	return GetCharacterMovement()->MaxWalkSpeed;
}


void ARetryCharacter::OnDeath()
{
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);
}

void ARetryCharacter::OnPlayerHealthChanged(float CurrentHP, float MaxHP)
{
	if (UFloatingNameWidget* Widget =
		Cast<UFloatingNameWidget>(HealthBarWidget->GetUserWidgetObject()))
	{
		Widget->SetHPPercent(CurrentHP / MaxHP);
	}
}

void ARetryCharacter::PlayHitReaction(FDamageInfo Info)
{
	if (!HitMontage) return;

	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		bIsStaggered = true;
		AnimInst->Montage_Play(HitMontage);
	}
}

void ARetryCharacter::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == HitMontage)
	{
		bIsStaggered = false;
	}
}
