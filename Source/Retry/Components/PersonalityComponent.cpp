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
    // 적용 전 스냅샷
    FPersonalitySnapshot Before = GetSnapshot();

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

    // 적용 후 — 캐릭터 이름 + 각 항목 Before→After 비교 로그
    FString OwnerName = GetOwner() ? GetOwner()->GetName() : TEXT("Unknown");

    // NPCName 있으면 그걸로 (더 읽기 쉬움)
    if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
    {
        if (ARetryNPCCharacter* NPC = Cast<ARetryNPCCharacter>(Char))
            OwnerName = NPC->NPCName;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Personality][%s] === 변화 요약 ==="), *OwnerName);

    auto LogChange = [&OwnerName](const TCHAR* Label, float Before, float After)
    {
        if (FMath::Abs(After - Before) < 0.001f) return;  // 변화 없으면 스킵

        const TCHAR* Arrow = (After > Before) ? TEXT("▲") : TEXT("▼");
        UE_LOG(LogTemp, Warning,
            TEXT("[Personality][%s]   %s: %.2f → %.2f (%s %.2f)"),
            *OwnerName, Label, Before, After, Arrow, FMath::Abs(After - Before));
    };

    LogChange(TEXT("Aggression"),      Before.Aggression,      Aggression);
    LogChange(TEXT("Fear"),            Before.Fear,            Fear);
    LogChange(TEXT("Trust"),           Before.Trust,           Trust);
    LogChange(TEXT("Courage"),         Before.Courage,         Courage);
    LogChange(TEXT("FearSensitivity"), Before.FearSensitivity, FearSensitivity);
    LogChange(TEXT("CoverPreference"), Before.CoverPreference, CoverPreference);
    LogChange(TEXT("TacticalSkill"),   Before.TacticalSkill,   TacticalSkill);
    LogChange(TEXT("TrustBias"),       Before.TrustBias,       TrustBias);
    LogChange(TEXT("Loyalty"),         Before.Loyalty,         Loyalty);
    LogChange(TEXT("Dominance"),       Before.Dominance,       Dominance);
    LogChange(TEXT("Patience"),        Before.Patience,        Patience);
}

FString UPersonalityComponent::BuildLLMPrompt(
    const TArray<FNPCMemory>& RecentMemories) const
{
    FString MemoryText;
    for (const FNPCMemory& Mem : RecentMemories)
    {
        MemoryText += FString::Printf(TEXT("- %s\n"), *Mem.Description);
    }

    FString PersonalityLabel;
    switch (DefaultPersonality)
    {
    case EPersonalityType::Aggressive:  PersonalityLabel = TEXT("aggressive and blunt"); break;
    case EPersonalityType::Cautious:    PersonalityLabel = TEXT("careful and nervous"); break;
    case EPersonalityType::Supportive:  PersonalityLabel = TEXT("warm and reassuring"); break;
    case EPersonalityType::Opportunist: PersonalityLabel = TEXT("calculating and pragmatic"); break;
    }

    return FString::Printf(TEXT(
           "You are simulating a game NPC's personality shift based on recent events.\n"
           "Current stats (0.0~1.0): Aggression=%.2f, Fear=%.2f, Trust=%.2f, "
           "Courage=%.2f, FearSensitivity=%.2f.\n"
           "This NPC's speaking tone is: %s.\n"
           "Recent events:\n%s\n"
           "Respond ONLY in JSON, no markdown, no explanation. "
           "Format: {\"Aggression\":0.0,\"Fear\":0.0,\"Trust\":0.0,"
           "\"Courage\":0.0,\"FearSensitivity\":0.0,"
           "\"Dialogue\":\"a short one-sentence in-character line reacting to the events, "
           "in Korean, max 15 words\"}\n"
           "Each numeric value is a DELTA (change amount, range -0.3 to 0.3), not absolute."
       ), Aggression, Fear, Trust, Courage, FearSensitivity, *PersonalityLabel, *MemoryText);
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

FPersonalitySnapshot UPersonalityComponent::GetSnapshot() const
{
    FPersonalitySnapshot Snap;
    Snap.Aggression      = Aggression;
    Snap.Fear            = Fear;
    Snap.Trust           = Trust;
    Snap.Courage         = Courage;
    Snap.FearSensitivity = FearSensitivity;
    Snap.CoverPreference = CoverPreference;
    Snap.TacticalSkill   = TacticalSkill;
    Snap.TrustBias       = TrustBias;
    Snap.Loyalty         = Loyalty;
    Snap.Dominance       = Dominance;
    Snap.Patience        = Patience;
    return Snap;
}