#include "RetryNPCController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Components/NPCDecisionComponent.h"

ARetryNPCController::ARetryNPCController(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer
        .SetDefaultSubobjectClass<UCrowdFollowingComponent>(
            TEXT("PathFollowingComponent")))
{
    PerceptionComponent =
        CreateDefaultSubobject<UAIPerceptionComponent>(
            TEXT("PerceptionComponent"));

    UAISenseConfig_Sight* SightConfig =
        CreateDefaultSubobject<UAISenseConfig_Sight>(
            TEXT("SightConfig"));
    SightConfig->SightRadius = 1500.f;
    SightConfig->LoseSightRadius = 2000.f;
    SightConfig->PeripheralVisionAngleDegrees = 90.f;
    SightConfig->DetectionByAffiliation.bDetectEnemies    = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals   = false;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

    PerceptionComponent->ConfigureSense(*SightConfig);
    PerceptionComponent->SetDominantSense(
        UAISense_Sight::StaticClass());

    // DecisionComponent 생성
    DecisionComponent = CreateDefaultSubobject<UNPCDecisionComponent>(
        TEXT("DecisionComponent"));
}

void ARetryNPCController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
        this, &ARetryNPCController::OnTargetPerceptionUpdated);

    if (BehaviorTreeAsset)
    {
        RunBehaviorTree(BehaviorTreeAsset);
        UE_LOG(LogTemp, Warning,
            TEXT("[NPC Controller] BT 실행: %s"),
            *BehaviorTreeAsset->GetName());
    }
}

void ARetryNPCController::OnUnPossess()
{
    PerceptionComponent->OnTargetPerceptionUpdated.RemoveAll(this);
    LastPerceivedActor = nullptr;

    Super::OnUnPossess();
}

ETeamAttitude::Type ARetryNPCController::GetTeamAttitudeTowards(const AActor& Other) const
{
    const IGenericTeamAgentInterface* OtherTeamAgent =
        Cast<IGenericTeamAgentInterface>(&Other);

    if (!OtherTeamAgent)
    {
        if (const APawn* OtherPawn = Cast<APawn>(&Other))
        {
            OtherTeamAgent = Cast<IGenericTeamAgentInterface>(
                OtherPawn->GetController());
        }
    }

    if (!OtherTeamAgent) return ETeamAttitude::Neutral;

    FGenericTeamId MyTeam = GetGenericTeamId();
    FGenericTeamId OtherTeam = OtherTeamAgent->GetGenericTeamId();

    // 같은 팀 — 우호
    if (MyTeam == OtherTeam)
        return ETeamAttitude::Friendly;

    // 플레이어(0)와 아군NPC(1)는 서로 우호
    bool bPlayerAllyPair =
        (MyTeam.GetId() == 0 && OtherTeam.GetId() == 1) ||
        (MyTeam.GetId() == 1 && OtherTeam.GetId() == 0);

    if (bPlayerAllyPair)
        return ETeamAttitude::Friendly;

    // 그 외(2 = 적)는 적대
    return ETeamAttitude::Hostile;
}


void ARetryNPCController::OnTargetPerceptionUpdated(
    AActor* Actor, FAIStimulus Stimulus)
{
    UE_LOG(LogTemp, Error, TEXT("[Perception] Sensed: %s | Actor: %s"),
    Stimulus.WasSuccessfullySensed() ? TEXT("TRUE") : TEXT("FALSE"),
    *Actor->GetName());
    
    if (Stimulus.WasSuccessfullySensed())
    {
        LastPerceivedActor = Actor;
        LastPerceptionTime = GetWorld()->GetTimeSeconds();
    }
    else
    {
        LastPerceivedActor = nullptr;
        LastLostTime = GetWorld()->GetTimeSeconds();
    }
}
