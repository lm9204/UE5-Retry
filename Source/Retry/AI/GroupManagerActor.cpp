#include "GroupManagerActor.h"

#include "AIController.h"
#include "LLMRequestQueue.h"
#include "RetryNPCCharacter.h"
#include "Components/NPCDecisionComponent.h"
#include "Components/PersonalityComponent.h"

AGroupManagerActor::AGroupManagerActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // 렌더링/충돌 없는 순수 데이터·로직 액터
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}

// ────────────────────────────────────────────────────
// 멤버 등록
// ────────────────────────────────────────────────────
void AGroupManagerActor::RegisterMember(
    ARetryNPCCharacter* NPC, bool bIsLeader)
{
    if (!NPC) return;

    if (bIsLeader)
    {
        Leader = NPC;
        UE_LOG(LogTemp, Warning,
            TEXT("[Group:%s] 리더 등록: %s"), *GroupID, *NPC->GetName());
    }

    Members.AddUnique(NPC);

    UE_LOG(LogTemp, Warning,
        TEXT("[Group:%s] 멤버 등록: %s (총 %d명)"),
        *GroupID, *NPC->GetName(), Members.Num());
}

// ────────────────────────────────────────────────────
// 그룹 메모리 기록 (목격자 기반)
// ────────────────────────────────────────────────────
void AGroupManagerActor::AddGroupMemory(
    const FString& WitnessID, const FString& EventType,
    FVector Location, float EmotionWeight, const FString& Description)
{
    FGroupMemoryEvent Event;
    Event.WitnessID = WitnessID;
    Event.EventType = EventType;
    Event.Location = Location;
    Event.Timestamp = GetWorld()->GetTimeSeconds();
    Event.EmotionWeight = EmotionWeight;
    Event.Description = Description;

    GroupMemories.Add(Event);
    AccumulatedEmotionScore += EmotionWeight;

    UE_LOG(LogTemp, Warning,
        TEXT("[Group:%s] %s 기록 (목격자: %s, Score: %.2f/%.2f)"),
        *GroupID, *Description, *WitnessID,
        AccumulatedEmotionScore, GroupEmotionThreshold);

    if (AccumulatedEmotionScore >= GroupEmotionThreshold)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Group:%s] 임계값 초과 — LLM 요청"), *GroupID);

        UGameInstance* GI = GetGameInstance();
        if (GI)
        {
            if (ULLMRequestQueue* Queue = GI->GetSubsystem<ULLMRequestQueue>())
            {
                Queue->EnqueueGroupRequest(this);  // 아래 추가
            }
        }

        AccumulatedEmotionScore = 0.f;
    }
}

TArray<FGroupMemoryEvent> AGroupManagerActor::GetRecentGroupMemories(
    int32 Count) const
{
    TArray<FGroupMemoryEvent> Result;
    int32 Start = FMath::Max(0, GroupMemories.Num() - Count);
    for (int32 i = Start; i < GroupMemories.Num(); i++)
        Result.Add(GroupMemories[i]);
    return Result;
}

// ────────────────────────────────────────────────────
// 리더 사망 처리
// ────────────────────────────────────────────────────
void AGroupManagerActor::OnLeaderDied()
{
    UE_LOG(LogTemp, Warning,
        TEXT("[Group:%s] 리더 사망 — 그룹 통제 불능"), *GroupID);

    Leader = nullptr;

    // 남은 부하 전원 즉시 명령 해제 → 개별 DecisionComponent 판단으로 전환
    SetOrderForAll(ENPCOrder::HoldFire, 0.f);
}

// ────────────────────────────────────────────────────
// 전체 멤버에게 명령 전파
// ────────────────────────────────────────────────────
void AGroupManagerActor::SetOrderForAll(ENPCOrder Order, float Weight)
{
    for (ARetryNPCCharacter* Member : Members)
    {
        if (!IsValid(Member)) continue;

        if (AAIController* AIC = Cast<AAIController>(Member->GetController()))
        {
            if (UNPCDecisionComponent* DC =
                AIC->FindComponentByClass<UNPCDecisionComponent>())
            {
                DC->SetOrder(Order, Weight);  // 아래 추가
            }
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[Group:%s] 전체 명령 전파: Weight=%.2f (%d명)"),
        *GroupID, Weight, Members.Num());
}

FString AGroupManagerActor::BuildGroupLLMPrompt() const
{
    FString MembersJson;
    for (int32 i = 0; i < Members.Num(); i++)
    {
        if (!IsValid(Members[i])) continue;

        UPersonalityComponent* PC =
            Members[i]->FindComponentByClass<UPersonalityComponent>();
        if (!PC) continue;

        MembersJson += FString::Printf(TEXT(
            "{\"ID\":\"%s\",\"Aggression\":%.2f,\"Fear\":%.2f,"
            "\"Trust\":%.2f,\"Courage\":%.2f,\"Loyalty\":%.2f}%s"
        ), *Members[i]->GetName(), PC->GetAggression(), PC->GetFear(),
           PC->GetTrust(), PC->GetCourage(), PC->GetLoyalty(),
           (i < Members.Num() - 1) ? TEXT(",") : TEXT(""));
    }

    FString MemoryText;
    for (const FGroupMemoryEvent& Mem : GetRecentGroupMemories(8))
    {
        MemoryText += FString::Printf(TEXT("- (목격: %s) %s\n"),
            *Mem.WitnessID, *Mem.Description);
    }

    return FString::Printf(TEXT(
        "You are commanding a small squad in a tactical game.\n"
        "Squad members:\n[%s]\n"
        "Recent events witnessed by the squad:\n%s\n"
        "Respond ONLY in JSON, no markdown. Format:\n"
        "{\"Members\":[{\"ID\":\"name\",\"Aggression\":0.0,\"Fear\":0.0,"
        "\"Trust\":0.0,\"Courage\":0.0,\"Loyalty\":0.0}],"
        "\"Order\":\"HoldFire|FreeFire|Charge|Retreat|TakeCover\"}\n"
        "Each numeric value is a DELTA (-0.3~0.3). "
        "IMPORTANT: identical events can shift different members' personalities "
        "in different directions depending on their existing traits — "
        "do not apply a uniform change to everyone."
    ), *MembersJson, *MemoryText);
}
