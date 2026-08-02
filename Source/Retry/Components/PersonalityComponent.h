#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PersonalityComponent.generated.h"

USTRUCT()
struct FPersonalitySnapshot
{
    GENERATED_BODY()

    float Aggression = 0.f;
    float Fear = 0.f;
    float Trust = 0.f;
    float Courage = 0.f;
    float FearSensitivity = 0.f;
    float CoverPreference = 0.f;
    float TacticalSkill = 0.f;
    float TrustBias = 0.f;
    float Loyalty = 0.f;
    float Dominance = 0.f;
    float Patience = 0.f;
};

UENUM(BlueprintType)
enum class EPersonalityType : uint8
{
    Aggressive  UMETA(DisplayName = "Aggressive"),
    Cautious    UMETA(DisplayName = "Cautious"),
    Supportive  UMETA(DisplayName = "Supportive"),
    Opportunist UMETA(DisplayName = "Opportunist"),
};

USTRUCT(BlueprintType)
struct FPersonalityDelta
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Aggression      = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Fear             = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Trust             = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Courage          = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FearSensitivity  = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CoverPreference  = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TacticalSkill    = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TrustBias        = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Loyalty           = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Dominance         = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Patience          = 0.f;
};

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RETRY_API UPersonalityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPersonalityComponent();

    FPersonalitySnapshot GetSnapshot() const;

    // ── 기존 getter ──────────────────────────────────
    UFUNCTION(BlueprintPure, Category = "Personality")
    EPersonalityType GetPersonalityType() const;

    UFUNCTION(BlueprintPure, Category = "Personality")
    float GetAggression() const { return Aggression; }

    UFUNCTION(BlueprintPure, Category = "Personality")
    float GetFear() const { return Fear; }

    UFUNCTION(BlueprintPure, Category = "Personality")
    float GetTrust() const { return Trust; }

    // ── 신규 getter ──────────────────────────────────
    UFUNCTION(BlueprintPure, Category = "Personality")
    float GetCourage() const { return Courage; }

    UFUNCTION(BlueprintPure, Category = "Personality")
    float GetFearSensitivity() const { return FearSensitivity; }

    UFUNCTION(BlueprintPure, Category = "Personality")
    float GetCoverPreference() const { return CoverPreference; }

    UFUNCTION(BlueprintPure, Category = "Personality")
    float GetTacticalSkill() const { return TacticalSkill; }

    UFUNCTION(BlueprintPure, Category = "Personality")
    float GetTrustBias() const { return TrustBias; }

    UFUNCTION(BlueprintPure, Category = "Personality")
    float GetLoyalty() const { return Loyalty; }

    UFUNCTION(BlueprintPure, Category = "Personality")
    float GetDominance() const { return Dominance; }

    UFUNCTION(BlueprintPure, Category = "Personality")
    float GetPatience() const { return Patience; }

    UFUNCTION(BlueprintPure, Category = "Personality")
    float GetStress() const { return Stress; }

    // ── 프리셋 / 동기화 ──────────────────────────────
    void ApplyPersonalityPreset();
    void SyncToBlackboard();

    // ── LLM 연동 ─────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Personality")
    void ApplyDelta(const FPersonalityDelta& Delta);

    UFUNCTION(BlueprintCallable, Category="Personality")
    FString BuildLLMPrompt(const TArray<struct FNPCMemory>& RecentMemories) const;

    // ── Stress 관리 ──────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Personality")
    void AddStress(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Personality")
    void RecoverStress(float DeltaTime);

    // ── 속성 ─────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    EPersonalityType DefaultPersonality = EPersonalityType::Opportunist;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    float Aggression = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    float Fear = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    float Trust = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    float Courage = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    float FearSensitivity = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    float CoverPreference = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    float TacticalSkill = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    float TrustBias = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    float Loyalty = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    float Dominance = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
    float Patience = 0.5f;

    // Stress — 런타임 전용, 초기값 0
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Personality|Stress")
    float Stress = 0.f;

    UPROPERTY(EditAnywhere, Category = "Personality|Stress")
    float StressRecoveryRate = 0.1f;

    UPROPERTY(EditAnywhere, Category = "Personality|Stress")
    float MaxStress = 1.0f;

protected:
    virtual void BeginPlay() override;
};