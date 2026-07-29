#pragma once

#include "CoreMinimal.h"
#include "NPCContext.generated.h"

UENUM(BlueprintType)
enum class ENPCCombatState : uint8
{
	Idle        UMETA(DisplayName = "Idle"),
	Patrol      UMETA(DisplayName = "Patrol"),
	Alert       UMETA(DisplayName = "Alert"),
	Search      UMETA(DisplayName = "Search"),
	Attack      UMETA(DisplayName = "Attack"),
	TakeCover   UMETA(DisplayName = "TakeCover"),
	Reload      UMETA(DisplayName = "Reload"),
	Retreat     UMETA(DisplayName = "Retreat"),
	Hold        UMETA(DisplayName = "Hold"),
	Suppress    UMETA(DisplayName = "Suppress"),
	Dead        UMETA(DisplayName = "Dead"),
};

USTRUCT()
struct FNPCContext
{
	GENERATED_BODY()

	// 적 관련
	AActor* Target            = nullptr;
	float   DistToTarget      = 99999.f;
	bool    bCanSeeTarget     = false;
	FVector LastKnownLocation = FVector::ZeroVector;
	bool    bTargetJustDied   = false;
	
	// 자신 상태
	float HPRatio      = 1.f;
	int32 CurrentAmmo  = 0;
	int32 MaxAmmo      = 30;
	int32 ReserveAmmo  = 0;
	bool  bIsReloading = false;
	bool  bIsInCover   = false;
	bool  bIsArmed	   = false;

	// 성격
	float Aggression      = 0.5f;
	float Fear            = 0.5f;
	float Trust           = 0.5f;
	float Courage         = 0.5f;
	float FearSensitivity = 0.5f;
	float CoverPreference = 0.5f;
	float TacticalSkill   = 0.5f;
	float TrustBias       = 0.5f;
	float Loyalty         = 0.5f;
	float Dominance       = 0.5f;
	float Patience        = 0.5f;
	float Stress          = 0.f;

	// 환경
	FVector CoverLocation = FVector::ZeroVector;
	bool    bAlerted      = false;
	bool    bHasPatrol    = false;
};

USTRUCT()
struct FNPCScoreSnapshot
{
	GENERATED_BODY()
	
	float AttackScore	= 0.f;
	float CoverScore	= 0.f;
	float RetreatScore	= 0.f;
	float ReloadScore	= 0.f;
	ENPCCombatState WinnerState = ENPCCombatState::Idle;
};

USTRUCT()
struct FNPCStateTransition
{
	GENERATED_BODY()
	ENPCCombatState	FromState	= ENPCCombatState::Idle;
	ENPCCombatState	ToState		= ENPCCombatState::Idle;
	FString			Reason;
	float			Timestamp = 0.f;
	FNPCScoreSnapshot ScoreAtTransition;
};
