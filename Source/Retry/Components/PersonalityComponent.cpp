#include "Components/PersonalityComponent.h"
#include "Components/MemoryComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Debug/CombatLogging.h"
#include "GameFramework/Character.h"

UPersonalityComponent::UPersonalityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPersonalityComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplyPersonalityPreset();
    SyncToBlackboard();
}

EPersonalityType UPersonalityComponent::GetPersonalityType() const
{
    return DefaultPersonality;
}

void UPersonalityComponent::SyncToBlackboard()
{
    ACharacter* Owner = Cast<ACharacter>(GetOwner());
    if (!Owner) return;

    AAIController* AIC = Cast<AAIController>(Owner->GetController());
    if (!AIC) return;

    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
    if (!BB) return;

    BB->SetValueAsFloat(TEXT("Aggression"), Aggression);
    BB->SetValueAsFloat(TEXT("Fear"),       Fear);
    BB->SetValueAsFloat(TEXT("Trust"),      Trust);
}

void UPersonalityComponent::ApplyPersonalityPreset()
{
    switch (DefaultPersonality)
    {
    case EPersonalityType::Aggressive:
        Aggression      = 0.85f; Fear            = 0.15f; Trust = 0.40f;
        Courage         = 0.80f; FearSensitivity = 0.10f;
        CoverPreference = 0.20f; TacticalSkill   = 0.40f;
        TrustBias       = 0.30f; Loyalty         = 0.50f;
        Dominance       = 0.80f; Patience        = 0.20f;
        break;

    case EPersonalityType::Cautious:
        Aggression      = 0.20f; Fear            = 0.70f; Trust = 0.50f;
        Courage         = 0.25f; FearSensitivity = 0.80f;
        CoverPreference = 0.80f; TacticalSkill   = 0.70f;
        TrustBias       = 0.50f; Loyalty         = 0.60f;
        Dominance       = 0.20f; Patience        = 0.70f;
        break;

    case EPersonalityType::Supportive:
        Aggression      = 0.20f; Fear            = 0.40f; Trust = 0.90f;
        Courage         = 0.50f; FearSensitivity = 0.50f;
        CoverPreference = 0.60f; TacticalSkill   = 0.50f;
        TrustBias       = 0.90f; Loyalty         = 0.80f;
        Dominance       = 0.40f; Patience        = 0.60f;
        break;

    case EPersonalityType::Opportunist:
        Aggression      = 0.50f; Fear            = 0.50f; Trust = 0.50f;
        Courage         = 0.50f; FearSensitivity = 0.50f;
        CoverPreference = 0.50f; TacticalSkill   = 0.60f;
        TrustBias       = 0.50f; Loyalty         = 0.50f;
        Dominance       = 0.50f; Patience        = 0.50f;
        break;
    }

    Stress = 0.f;
}

void UPersonalityComponent::ApplyDelta(const FPersonalityDelta& Delta)
{
    auto ClampAdd = [](float& Val, float D)
    {
        Val = FMath::Clamp(Val + D, 0.f, 1.f);
    };

    ClampAdd(Aggression,      Delta.Aggression);
    ClampAdd(Fear,            Delta.Fear);
    ClampAdd(Trust,           Delta.Trust);
    ClampAdd(Courage,         Delta.Courage);
    ClampAdd(FearSensitivity, Delta.FearSensitivity);
    ClampAdd(CoverPreference, Delta.CoverPreference);
    ClampAdd(TacticalSkill,   Delta.TacticalSkill);
    ClampAdd(TrustBias,       Delta.TrustBias);
    ClampAdd(Loyalty,         Delta.Loyalty);
    ClampAdd(Dominance,       Delta.Dominance);
    ClampAdd(Patience,        Delta.Patience);

    SyncToBlackboard();

    UE_LOG(LogTemp, Warning, TEXT("[Personality] ApplyDelta 적용 — %s"),
        *GetCombatLogName(GetOwner()));
}

FString UPersonalityComponent::BuildLLMPrompt(
    const TArray<FNPCMemory>& RecentMemories) const
{
    FString MemoryText;
    for (const FNPCMemory& Mem : RecentMemories)
    {
        MemoryText += FString::Printf(TEXT("- %s\n"), *Mem.Description);
    }

    return FString::Printf(TEXT(
        "You are simulating a game NPC's personality shift based on recent events.\n"
        "Current stats (0.0~1.0): Aggression=%.2f, Fear=%.2f, Trust=%.2f, "
        "Courage=%.2f, FearSensitivity=%.2f.\n"
        "Recent events:\n%s\n"
        "Respond ONLY in JSON, no markdown, no explanation. "
        "Format: {\"Aggression\":0.0,\"Fear\":0.0,\"Trust\":0.0,"
        "\"Courage\":0.0,\"FearSensitivity\":0.0}\n"
        "Each value is a DELTA (change amount, range -0.3 to 0.3), not absolute."
    ), Aggression, Fear, Trust, Courage, FearSensitivity, *MemoryText);
}

void UPersonalityComponent::AddStress(float Amount)
{
    Stress = FMath::Clamp(Stress + Amount, 0.f, MaxStress);
}

void UPersonalityComponent::RecoverStress(float DeltaTime)
{
    if (Stress <= 0.f) return;
    Stress = FMath::Clamp(Stress - StressRecoveryRate * DeltaTime,
        0.f, MaxStress);
}