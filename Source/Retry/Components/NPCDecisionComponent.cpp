#include "NPCDecisionComponent.h"

#include "AIController.h"
#include "MemoryComponent.h"
#include "NavigationSystem.h"
#include "RetryNPCController.h"
#include "RetryNPCCharacter.h"
#include "AI/GroupManagerActor.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/HealthComponent.h"
#include "Components/PersonalityComponent.h"
#include "Components/WeaponComponent.h"
#include "Debug/AIDebugWidget.h"
#include "Debug/CombatLogging.h"
#include "GameFramework/Character.h"

namespace MissionBlackboard
{
	const FName TargetLocationKey = TEXT("MissionTargetLocation");
	const FName MovementAllowedKey = TEXT("bMissionMovementAllowed");
}

UNPCDecisionComponent::UNPCDecisionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// ────────────────────────────────────────────────────
// 진입점
// ────────────────────────────────────────────────────
void UNPCDecisionComponent::Update(float DeltaSeconds)
{
    TimeSinceChange += DeltaSeconds;

    UpdateAIState(DeltaSeconds);
    UpdateCombatState();
    UpdateDialogueState();
    WriteBlackboard();

#if !UE_BUILD_SHIPPING
    UpdateDebugWidget();
#endif
}

// ────────────────────────────────────────────────────
// 1. UpdateAIState — Perception + TargetActor 관리
// ────────────────────────────────────────────────────
void UNPCDecisionComponent::UpdateAIState(float DeltaSeconds)
{
    AAIController* AIC = Cast<AAIController>(
        GetOwner()->GetInstigatorController());
    if (!AIC) return;

    ARetryNPCController* NPCC = Cast<ARetryNPCController>(AIC);
    if (!NPCC) return;

    ACharacter* NPC = Cast<ACharacter>(AIC->GetPawn());
    if (!NPC) return;

    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
    if (!BB) return;

    // if (NPCC->LastPerceivedActor)
    // {
    //     bool bCanSee = CheckLineOfSight(NPC, NPCC->LastPerceivedActor);
    //
    //     if (bCanSee)
    //     {
    //         BB->SetValueAsObject(TEXT("TargetActor"),
    //             NPCC->LastPerceivedActor);
    //         BB->SetValueAsBool(TEXT("bAlerted"), true);
    //         TimeSinceLostSight = 0.f;
    //     }
    //     else
    //     {
    //         TimeSinceLostSight += DeltaSeconds;
    //         if (TimeSinceLostSight >= LostSightTimeout)
    //         {
    //             BB->ClearValue(TEXT("TargetActor"));
    //             BB->SetValueAsBool(TEXT("bAlerted"), false);
    //             NPCC->LastPerceivedActor = nullptr;
    //             TimeSinceLostSight = 0.f;
    //         }
    //     }
    // }
    // ===임시===
    if (NPCC->LastPerceivedActor)
    {
        // bCanSeeTarget은 여기서 true로 단정하지 않는다 — LastPerceivedActor는
        // 시야 차단 후에도 LostSightTimeout 동안 유지되므로, 실제 가시성은
        // CollectContext()의 CheckLineOfSight 결과를 WriteBlackboard()가 반영한다.
        BB->SetValueAsObject(TEXT("TargetActor"), NPCC->LastPerceivedActor);
        BB->SetValueAsBool(TEXT("bAlerted"), true);
        TimeSinceLostSight = 0.f;
    }
    else
    {
        TimeSinceLostSight += DeltaSeconds;
        if (TimeSinceLostSight >= LostSightTimeout)
        {
            BB->ClearValue(TEXT("TargetActor"));
            BB->SetValueAsBool(TEXT("bAlerted"), false);
            TimeSinceLostSight = 0.f;
        }
    }
}

// ────────────────────────────────────────────────────
// 2. UpdateCombatState — 점수 기반 상태 결정
// ────────────────────────────────────────────────────
void UNPCDecisionComponent::UpdateCombatState()
{
    CachedContext = CollectContext();
    ENPCCombatState Candidate = DecideState(CachedContext);
    ENPCCombatState Stable    = GetStableState(Candidate, CachedContext);

    bool bWasFiringState =
        CurrentState == ENPCCombatState::Attack ||
        CurrentState == ENPCCombatState::TakeCover ||
        CurrentState == ENPCCombatState::Suppress;

    bool bStillFiringState =
        Stable == ENPCCombatState::Attack ||
        Stable == ENPCCombatState::TakeCover ||
        Stable == ENPCCombatState::Suppress;

    bool bNowFiringState = bStillFiringState;

    if (ACharacter* NPC = Cast<ACharacter>(
           Cast<AAIController>(GetOwner()->GetInstigatorController())->GetPawn()))
    {
        if (UWeaponComponent* WC = NPC->FindComponentByClass<UWeaponComponent>())
        {
            if (!bWasFiringState && bNowFiringState)
            {
                WC->StartAim();  // Attack/TakeCover 진입 시 한 번만
            }

            if (bWasFiringState && !bStillFiringState)
            {
                WC->StopAim();
            }
        }
    }

    if (Stable != CurrentState)
    {
        // 전투 결과 메모리 기록
        if (ACharacter* NPC = Cast<ACharacter>(
            Cast<AAIController>(GetOwner()->GetInstigatorController())->GetPawn()))
        {
            if (UMemoryComponent* Memory = NPC->FindComponentByClass<UMemoryComponent>())
            {
                if (Stable == ENPCCombatState::Retreat)
                {
                    Memory->AddMemory(EMemoryEventType::CombatDefeat,
                        NPC->GetActorLocation(), 0.5f,
                        TEXT("전투에서 밀려 후퇴함"));

                    if (UPersonalityComponent* PC =
                        NPC->FindComponentByClass<UPersonalityComponent>())
                    {
                        PC->Fear = 1.0f;   // 최대치 고정 — 재진입 방지
                    }

                    // 레벨 퇴장 처리 — 일정 시간 후 비활성화
                    FTimerHandle RetreatExitHandle;
                    TWeakObjectPtr<ACharacter> WeakNPC(NPC);
                    GetWorld()->GetTimerManager().SetTimer(RetreatExitHandle,
                        [WeakNPC]()
                        {
                            if (WeakNPC.IsValid())
                            {
                                WeakNPC->SetActorHiddenInGame(true);
                                WeakNPC->SetActorEnableCollision(false);
                                WeakNPC->SetActorTickEnabled(false);
                            }
                        }, 5.f, false);  // 5초 후 완전 퇴장 (도주 애니메이션 볼 시간 확보)
                }
                else if (CachedContext.bTargetJustDied &&
                         (bWasFiringState || CurrentState == ENPCCombatState::Reload))
                {
                    Memory->AddMemory(EMemoryEventType::EnemyKilled,
                        NPC->GetActorLocation(), 0.2f,
                        TEXT("교전 후 적을 제압함"));
                }
            }
        }
        
        FString Reason = FString::Printf(
            TEXT("A:%.2f C:%.2f R:%.2f Rl:%.2f"),
            LastScoreSnapshot.AttackScore,
            LastScoreSnapshot.CoverScore,
            LastScoreSnapshot.RetreatScore,
            LastScoreSnapshot.ReloadScore);

        RecordTransition(CurrentState, Stable, Reason);
        CurrentState    = Stable;
        TimeSinceChange = 0.f;
    }
}

// ────────────────────────────────────────────────────
// 3. UpdateDialogueState — LLM 대비 스텁
// ────────────────────────────────────────────────────
void UNPCDecisionComponent::UpdateDialogueState()
{
    // TODO (5~6주차)
    // MemoryComponent 누적값 확인
    // 임계값 초과 시 LLMRequestQueue에 요청
    // 응답 오면 PersonalityComponent.ApplyDelta()
}

// ────────────────────────────────────────────────────
// 4. WriteBlackboard — 결정 결과 BB에 일괄 기록
// ────────────────────────────────────────────────────
void UNPCDecisionComponent::WriteBlackboard()
{
    AAIController* AIC = Cast<AAIController>(
        GetOwner()->GetInstigatorController());
    if (!AIC) return;

    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
    if (!BB) return;

    BB->SetValueAsEnum(TEXT("CombatState"),
        static_cast<uint8>(CurrentState));

    // Personality 수치 동기화 (LLM 연동 대비)
    BB->SetValueAsFloat(TEXT("Aggression"), CachedContext.Aggression);
    BB->SetValueAsFloat(TEXT("Fear"),       CachedContext.Fear);
    BB->SetValueAsFloat(TEXT("Trust"),      CachedContext.Trust);
    BB->SetValueAsFloat(TEXT("TrustBias"),  CachedContext.TrustBias);
    BB->SetValueAsFloat(TEXT("Loyalty"),    CachedContext.Loyalty);

    // 엄폐 위치
    // if (!CachedContext.CoverLocation.IsZero())
        BB->SetValueAsVector(TEXT("CoverLocation"),
            CachedContext.CoverLocation);

    // LastKnownLocation — 볼 수 있을 때만 갱신
    if (CachedContext.bCanSeeTarget && CachedContext.Target)
        BB->SetValueAsVector(TEXT("LastKnownEnemyLocation"),
            CachedContext.Target->GetActorLocation());

	BB->SetValueAsBool(
		MissionBlackboard::MovementAllowedKey,
		IsMissionMovementAllowedForState(CurrentState));
	if (bHasActiveMission)
	{
		BB->SetValueAsVector(
			MissionBlackboard::TargetLocationKey,
			ActiveMissionContext.ObjectiveLocation);
	}
	else
	{
		BB->ClearValue(MissionBlackboard::TargetLocationKey);
	}
}

// ────────────────────────────────────────────────────
// Context 수집
// ────────────────────────────────────────────────────
FNPCContext UNPCDecisionComponent::CollectContext()
{
    FNPCContext Ctx;

    AAIController* AIC = Cast<AAIController>(
        GetOwner()->GetInstigatorController());
    if (!AIC) return Ctx;

    ACharacter* NPC = Cast<ACharacter>(AIC->GetPawn());
    if (!NPC) return Ctx;

    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
    if (!BB) return Ctx;

    // 적
    Ctx.Target = Cast<AActor>(
        BB->GetValueAsObject(TEXT("TargetActor")));
    Ctx.bAlerted = BB->GetValueAsBool(TEXT("bAlerted"));

    if (Ctx.Target && IsValid(Ctx.Target))
    {
        if (UHealthComponent* TargetHC =
            Ctx.Target->FindComponentByClass<UHealthComponent>())
        {
            if (TargetHC->IsDead())
            {
                // 죽은 타겟 즉시 제거
                BB->ClearValue(TEXT("TargetActor"));
                Ctx.Target = nullptr;
                Ctx.bAlerted = false;
                Ctx.bTargetJustDied = true;
                BB->SetValueAsBool(TEXT("bAlerted"), false);

                if (ARetryNPCController* NPCC = Cast<ARetryNPCController>(AIC))
                {
                    NPCC->LastPerceivedActor = nullptr;
                }
            }
        }

        if (Ctx.Target)
        {
            bool bWasEmpty = !bWasTargetSetLastTick;

            Ctx.DistToTarget = FVector::Dist(
                NPC->GetActorLocation(),
                Ctx.Target->GetActorLocation());
            Ctx.bCanSeeTarget = CheckLineOfSight(NPC, Ctx.Target);
            Ctx.LastKnownLocation =
                BB->GetValueAsVector(TEXT("LastKnownEnemyLocation"));

            if (Ctx.bCanSeeTarget && bWasEmpty)
            {
                ARetryNPCCharacter* NPCChar = Cast<ARetryNPCCharacter>(NPC);
                if (NPCChar && NPCChar->MyGroup)
                {
                    NPCChar->MyGroup->AddGroupMemory(
                        NPCChar->NPCName,
                        TEXT("CombatStart"),
                        NPC->GetActorLocation(),
                        0.1f,
                        FString::Printf(TEXT("%s가 적을 발견하며 교전이 시작됨"),
                            *NPCChar->NPCName)
                    );
                }
            }
            bWasTargetSetLastTick = true;
        }
    }
    else
    {
        bWasTargetSetLastTick = false;
    }

    // HP
    if (UHealthComponent* HC =
        NPC->FindComponentByClass<UHealthComponent>())
    {
        Ctx.HPRatio = HC->GetCurrentHealth() / HC->MaxHealth;
    }

    // 탄약
    if (UWeaponComponent* WC =
        NPC->FindComponentByClass<UWeaponComponent>())
    {
        Ctx.CurrentAmmo  = WC->GetCurrentAmmo();
        Ctx.MaxAmmo      = WC->GetMagSize();
        Ctx.ReserveAmmo  = WC->GetReserveAmmo();
        Ctx.bIsReloading = WC->GetIsReloading();
        Ctx.bIsArmed = WC->IsArmed();
    }

    // 성격
    if (UPersonalityComponent* PC =
        NPC->FindComponentByClass<UPersonalityComponent>())
    {
        Ctx.Aggression      = PC->GetAggression();
        Ctx.Fear            = PC->GetFear();
        Ctx.Trust           = PC->GetTrust();
        Ctx.Courage         = PC->GetCourage();
        Ctx.FearSensitivity = PC->GetFearSensitivity();
        Ctx.CoverPreference = PC->GetCoverPreference();
        Ctx.TacticalSkill   = PC->GetTacticalSkill();
        Ctx.TrustBias       = PC->GetTrustBias();
        Ctx.Loyalty         = PC->GetLoyalty();
        Ctx.Dominance       = PC->GetDominance();
        Ctx.Patience        = PC->GetPatience();
        Ctx.Stress          = PC->GetStress();

        // Stress 회복 — 전투 외
        if (!Ctx.Target)
            PC->RecoverStress(GetWorld()->GetDeltaSeconds());
    }

    // 순찰
    if (ARetryNPCCharacter* NPCChar =
        Cast<ARetryNPCCharacter>(NPC))
    {
        Ctx.bHasPatrol = NPCChar->PatrolPoints.Num() > 0;
    }

    // 엄폐
    Ctx.CoverLocation = BB->GetValueAsVector(TEXT("CoverLocation"));
    Ctx.bIsInCover = !Ctx.CoverLocation.IsZero() &&
        FVector::Dist(NPC->GetActorLocation(),
            Ctx.CoverLocation) < 100.f;

    // 그룹 명령
    Ctx.CurrentOrder = PendingOrder;
    Ctx.OrderWeight = PendingOrderWeight;

    // 커버 탐색
    if (Ctx.Target)
    {
        TimeSinceCoverSearch += GetWorld()->GetDeltaSeconds();
        if (TimeSinceCoverSearch >= CoverSearchInterval)
        {
            FindBestCoverLocation(NPC, Ctx.Target, Ctx);
            TimeSinceCoverSearch = 0.f;
        }
    }

    return Ctx;
}

// ────────────────────────────────────────────────────
// 상태 결정
// ────────────────────────────────────────────────────
ENPCCombatState UNPCDecisionComponent::DecideState(
    const FNPCContext& Ctx)
{
    // Dead
    if (Ctx.HPRatio <= 0.f)
        return ENPCCombatState::Dead;

    // 전투 없을 때
    if (!Ctx.Target)
    {
        // 평시에도 탄약이 가득 차지 않았고 여유 탄약이 있으면 미리 장전
        if (Ctx.bIsArmed && !Ctx.bIsReloading &&
            Ctx.CurrentAmmo < Ctx.MaxAmmo && Ctx.ReserveAmmo > 0)
            return ENPCCombatState::Reload;

        if (Ctx.bAlerted)
            return ENPCCombatState::Search;
        if (Ctx.bHasPatrol)
            return ENPCCombatState::Patrol;
        return ENPCCombatState::Idle;
    }

    // 전투 중 — 점수 기반
    float AttackScore  = FMath::Clamp(
        CalcAttackScore(Ctx)  * AttackScoreWeight,  0.f, 1.f);
    float CoverScore   = FMath::Clamp(
        CalcCoverScore(Ctx)   * CoverScoreWeight,   0.f, 1.f);
    float RetreatScore = FMath::Clamp(
        CalcRetreatScore(Ctx) * RetreatScoreWeight, 0.f, 1.f);
    float ReloadScore  = FMath::Clamp(
        CalcReloadScore(Ctx)  * ReloadScoreWeight,  0.f, 1.f);

    // 스냅샷 저장
    LastScoreSnapshot.AttackScore  = AttackScore;
    LastScoreSnapshot.CoverScore   = CoverScore;
    LastScoreSnapshot.RetreatScore = RetreatScore;
    LastScoreSnapshot.ReloadScore  = ReloadScore;

    // 최고 점수 선택
    float Max = FMath::Max(
        FMath::Max(AttackScore, CoverScore),
        FMath::Max(RetreatScore, ReloadScore));

    if (Max == ReloadScore)
    {
        LastScoreSnapshot.WinnerState = ENPCCombatState::Reload;
        return ENPCCombatState::Reload;
    }
    if (Max == RetreatScore)
    {
        LastScoreSnapshot.WinnerState = ENPCCombatState::Retreat;
        return ENPCCombatState::Retreat;
    }
    if (Max == CoverScore)
    {
        // 이미 엄폐 중이고 타겟이 보이면 제압사격, 그 외(이동/은신 중)에는 TakeCover
        if (Ctx.bIsInCover && Ctx.bCanSeeTarget)
        {
            LastScoreSnapshot.WinnerState = ENPCCombatState::Suppress;
            return ENPCCombatState::Suppress;
        }
        LastScoreSnapshot.WinnerState = ENPCCombatState::TakeCover;
        return ENPCCombatState::TakeCover;
    }

    LastScoreSnapshot.WinnerState = ENPCCombatState::Attack;
    return ENPCCombatState::Attack;
}

// ────────────────────────────────────────────────────
// 상태 안정화
// ────────────────────────────────────────────────────
ENPCCombatState UNPCDecisionComponent::GetStableState(
    ENPCCombatState Candidate, const FNPCContext& Ctx)
{
    // 즉시 전환
    if (Candidate == ENPCCombatState::Dead ||
        Candidate == ENPCCombatState::Reload)
        return Candidate;

    // 강제 종료 조건
    if (ShouldForceExit(CurrentState, Ctx))
        return Candidate;

    // 시간 기반 안정화
    if (TimeSinceChange < StateMinDuration)
        return CurrentState;

    return Candidate;
}

bool UNPCDecisionComponent::ShouldForceExit(
    ENPCCombatState State, const FNPCContext& Ctx)
{
    switch (State)
    {
    case ENPCCombatState::Attack:
        return !Ctx.Target || Ctx.CurrentAmmo <= 0 || !Ctx.bCanSeeTarget;
    case ENPCCombatState::Retreat:
        return !Ctx.Target && Ctx.HPRatio > 0.5f;
    case ENPCCombatState::TakeCover:
        return !Ctx.Target;
    case ENPCCombatState::Suppress:
        return !Ctx.Target || Ctx.CurrentAmmo <= 0 || !Ctx.bCanSeeTarget;
    default:
        return false;
    }
}

// ────────────────────────────────────────────────────
// 점수 계산
// ────────────────────────────────────────────────────
float UNPCDecisionComponent::CalcAttackScore(const FNPCContext& Ctx)
{
    float Score = 0.f;
    if (!Ctx.bIsArmed) return 0.f;
    if (Ctx.CurrentAmmo <= 0) return 0.f;
    if (!Ctx.bCanSeeTarget) return 0.f;
    Score += 0.4f;
    Score += Ctx.Aggression  * 0.3f;
    Score += Ctx.Courage     * 0.2f;
    Score += Ctx.Dominance   * 0.15f;
    Score -= (1.f - Ctx.HPRatio) * (1.f - Ctx.Courage) * 0.3f;
    Score -= Ctx.CoverPreference * 0.1f;

    // 명령 가중치 반영
    if (Ctx.CurrentOrder == ENPCOrder::Charge ||
        Ctx.CurrentOrder == ENPCOrder::FreeFire)
    {
        Score += Ctx.OrderWeight;
    }
    else if (Ctx.CurrentOrder == ENPCOrder::HoldFire)
    {
        Score -= Ctx.OrderWeight;
    }
    
    return FMath::Clamp(Score, 0.f, 1.f);
}

float UNPCDecisionComponent::CalcCoverScore(const FNPCContext& Ctx)
{
    if (Ctx.CoverLocation.IsZero())
        return 0.f;
    
    float Score = 0.f;
    Score += Ctx.CoverPreference * 0.4f;
    Score += Ctx.FearSensitivity * 0.2f;
    Score += Ctx.Fear            * 0.2f;
    Score += (1.f - Ctx.HPRatio) * 0.2f;

    // 명령 가중치 반영
    if (Ctx.CurrentOrder == ENPCOrder::TakeCover ||
        Ctx.CurrentOrder == ENPCOrder::HoldFire)
    {
        Score += Ctx.OrderWeight;
    }
    
    return FMath::Clamp(Score, 0.f, 1.f);
}

float UNPCDecisionComponent::CalcRetreatScore(const FNPCContext& Ctx)
{
    float Score = 0.f;
    Score += (1.f - Ctx.HPRatio) * 0.4f;
    Score += Ctx.Fear            * 0.2f;
    Score += Ctx.FearSensitivity * 0.2f;
    Score -= Ctx.Courage         * 0.3f;
    Score -= Ctx.Aggression      * 0.2f;

    Score -= Ctx.Loyalty * 0.15f;

    // 명령 가중치 반영
    if (Ctx.CurrentOrder == ENPCOrder::Retreat)
    {
        Score += Ctx.OrderWeight;
    }

    // 탄약도 예비 탄약도 없으면 더 이상 싸울 방법이 없으니 후퇴를 강하게 민다
    if (Ctx.bIsArmed && Ctx.CurrentAmmo <= 0 && Ctx.ReserveAmmo <= 0)
        Score += 0.6f;

    return FMath::Clamp(Score, 0.f, 1.f);
}

float UNPCDecisionComponent::CalcReloadScore(const FNPCContext& Ctx)
{
    if (!Ctx.bIsArmed) return 0.f;
    if (Ctx.ReserveAmmo <= 0) return 0.f;
    if (Ctx.CurrentAmmo <= 0) return 1.0f;
    float AmmoRatio = static_cast<float>(Ctx.CurrentAmmo)
        / FMath::Max(Ctx.MaxAmmo, 1);
    return FMath::Clamp((1.f - AmmoRatio) * 0.5f, 0.f, 1.f);
}

// ────────────────────────────────────────────────────
// 유틸
// ────────────────────────────────────────────────────
bool UNPCDecisionComponent::CheckLineOfSight(
    APawn* NPC, AActor* Target)
{
    if (!NPC || !Target) return false;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(NPC);
    Params.AddIgnoredActor(Target);

    FVector Start = NPC->GetActorLocation() + FVector(0,0,60.f);
    FVector End   = Target->GetActorLocation() + FVector(0,0,60.f);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit, Start, End, ECC_Visibility, Params);

    return !bHit && FVector::Dist(NPC->GetActorLocation(),
        Target->GetActorLocation()) <= SightRange;
}

void UNPCDecisionComponent::FindBestCoverLocation(
    APawn* NPC, AActor* Target, FNPCContext& Ctx)
{
    UNavigationSystemV1* NavSys =
        FNavigationSystem::GetCurrent<UNavigationSystemV1>(
            GetWorld());
    if (!NavSys) return;

    AAIController* AIC = Cast<AAIController>(
        GetOwner()->GetInstigatorController());
    if (!AIC) return;

    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
    if (!BB) return;

    FVector Best  = FVector::ZeroVector;
    float BestScore = -1.f;

    for (int32 i = 0; i < CoverTestPoints; i++)
    {
        FNavLocation NavLoc;
        if (!NavSys->GetRandomReachablePointInRadius(
            NPC->GetActorLocation(), CoverSearchRadius, NavLoc))
            continue;

        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(NPC);

        bool bBlocked = GetWorld()->LineTraceSingleByChannel(
            Hit,
            Target->GetActorLocation() + FVector(0,0,60.f),
            NavLoc.Location + FVector(0,0,60.f),
            ECC_Visibility, Params);

        if (!bBlocked) continue;

        float DistEnemy = FVector::Dist(
            NavLoc.Location, Target->GetActorLocation());
        float DistSelf  = FVector::Dist(
            NavLoc.Location, NPC->GetActorLocation());

        float Score = FMath::Clamp(DistEnemy, 300.f, 800.f)
            / 800.f - DistSelf / CoverSearchRadius;

        if (Score > BestScore)
        {
            BestScore = Score;
            Best = NavLoc.Location;
        }
    }
    
    if (BestScore > 0.f)
        Ctx.CoverLocation = Best;
    else
        Ctx.CoverLocation = FVector::ZeroVector;
}

void UNPCDecisionComponent::RecordTransition(
    ENPCCombatState From, ENPCCombatState To,
    const FString& Reason)
{
    FNPCStateTransition Entry;
    Entry.FromState         = From;
    Entry.ToState           = To;
    Entry.Reason            = Reason;
    Entry.Timestamp         = GetWorld()->GetTimeSeconds();
    Entry.ScoreAtTransition = LastScoreSnapshot;

    TransitionHistory.Add(Entry);
    if (TransitionHistory.Num() > MaxHistorySize)
        TransitionHistory.RemoveAt(0);

    FString NPCName = TEXT("?");
    if (AAIController* AIC = Cast<AAIController>(GetOwner()->GetInstigatorController()))
    {
        if (APawn* NPC = AIC->GetPawn())
            NPCName = GetCombatLogName(NPC);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[AI] %s: %s → %s | %s"),
        *NPCName,
        *UEnum::GetValueAsString(From),
        *UEnum::GetValueAsString(To),
        *Reason);
}

void UNPCDecisionComponent::UpdateDebugWidget()
{
    if (!bDebugEnabled) return;

    AAIController* AIC = Cast<AAIController>(
        GetOwner()->GetInstigatorController());
    if (!AIC) return;

    ARetryNPCCharacter* NPC =
        Cast<ARetryNPCCharacter>(AIC->GetPawn());
    if (!NPC || !NPC->AIDebugWidget) return;

    UAIDebugWidget* Widget =
        Cast<UAIDebugWidget>(NPC->AIDebugWidget->GetUserWidgetObject());
    if (!Widget) return;

    FString StateName = UEnum::GetValueAsString(CurrentState);
    FString TargetName = CachedContext.Target ?
        CachedContext.Target->GetName() : TEXT("None");

    Widget->UpdateDebugInfo(
        StateName,
        LastScoreSnapshot.AttackScore,
        LastScoreSnapshot.CoverScore,
        LastScoreSnapshot.RetreatScore,
        LastScoreSnapshot.ReloadScore,
        CachedContext.HPRatio,
        CachedContext.CurrentAmmo,
        TargetName,
        CachedContext.DistToTarget
    );
}

void UNPCDecisionComponent::SetOrder(ENPCOrder Order, float Weight)
{
    PendingOrder = Order;
    PendingOrderWeight = Weight;
}

bool UNPCDecisionComponent::SetMissionContext(
	const FMissionContext& MissionContext)
{
	if (!MissionContext.CommandId.IsValid()
		|| MissionContext.ObjectiveId.IsNone()
		|| MissionContext.ObjectiveLocation.ContainsNaN())
	{
		return false;
	}

	ActiveMissionContext = MissionContext;
	bHasActiveMission = true;
	return true;
}

void UNPCDecisionComponent::ClearMissionContext()
{
	ActiveMissionContext = FMissionContext();
	bHasActiveMission = false;
}

bool UNPCDecisionComponent::HasActiveMission() const
{
	return bHasActiveMission;
}

FMissionContext UNPCDecisionComponent::GetActiveMissionContext() const
{
	return bHasActiveMission ? ActiveMissionContext : FMissionContext();
}

bool UNPCDecisionComponent::IsMissionMovementAllowedForState(
	const ENPCCombatState State) const
{
	return bHasActiveMission
		&& (State == ENPCCombatState::Idle
			|| State == ENPCCombatState::Patrol);
}
