#include "LLMRequestQueue.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "RetryNPCCharacter.h"
#include "AI/GroupManagerActor.h"
#include "AI/NPCOrderTypes.h"
#include "Components/DialogComponent.h"
#include "Components/HealthComponent.h"
#include "Components/PersonalityComponent.h"
#include "Engine/World.h"
#include "UI/FallbackDialogueTypes.h"

void ULLMRequestQueue::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FallbackDialogueTable = LoadObject<UDataTable>(nullptr,
        TEXT("/Game/UI/DT_FallbackDialogue.DT_FallbackDialogue"));

    if (!FallbackDialogueTable)
    {
        UE_LOG(LogTemp, Error, TEXT("[LLMQueue] FallbackDialogueTable 로드 실패"));
    }
}

void ULLMRequestQueue::Enqueue(const FLLMRequest& Request)
{
    FLLMRequest RequestCopy = Request;

    RequestCopy.RequestID = NextRequestID++;

    UE_LOG(LogTemp, Warning,
        TEXT("[LLMQueue][#%d] 큐에 추가됨 — Type:%s"),
        RequestCopy.RequestID,
        RequestCopy.RequestType == ELLMRequestType::GroupCommand
            ? TEXT("Group") : TEXT("Individual"));

    PendingRequests.Enqueue(RequestCopy);

    if (!bIsProcessing)
        ProcessNext();
}

void ULLMRequestQueue::ProcessNext()
{
    FLLMRequest Request;
    if (!PendingRequests.Dequeue(Request))
    {
        bIsProcessing = false;
        return;
    }

    bIsProcessing = true;
    UE_LOG(LogTemp, Warning,
        TEXT("[LLMQueue][#%d] 큐에서 꺼냄 — 처리 시작"), Request.RequestID);

    SendRequest(Request);
}

void ULLMRequestQueue::SendRequest(const FLLMRequest& Request)
{
    bool bIsGroupRequest = (Request.RequestType == ELLMRequestType::GroupCommand);

    if (bIsGroupRequest && !Request.TargetGroup.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[LLMQueue][#%d] 그룹 대상 무효 — 스킵"), Request.RequestID);
        ProcessNext();
        return;
    }
    if (!bIsGroupRequest && !Request.TargetPersonality.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[LLMQueue][#%d] 개별 대상 무효 — 스킵"), Request.RequestID);
        ProcessNext();
        return;
    }

    float SendTime = GetWorld()->GetTimeSeconds();
    UE_LOG(LogTemp, Warning,
        TEXT("[LLMQueue][#%d] 전송 시작 — Type:%s, PromptLen:%d, WorldTime:%.2f"),
        Request.RequestID,
        bIsGroupRequest ? TEXT("Group") : TEXT("Individual"),
        Request.Prompt.Len(), SendTime);

    TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(ServerURL);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // JSON Body 구성
    TSharedPtr<FJsonObject> RootObj = MakeShared<FJsonObject>();
    RootObj->SetNumberField(TEXT("temperature"), 0.3);

    TArray<TSharedPtr<FJsonValue>> Messages;
    TSharedPtr<FJsonObject> MsgObj = MakeShared<FJsonObject>();
    MsgObj->SetStringField(TEXT("role"), TEXT("user"));
    MsgObj->SetStringField(TEXT("content"), Request.Prompt);
    Messages.Add(MakeShared<FJsonValueObject>(MsgObj));
    RootObj->SetArrayField(TEXT("messages"), Messages);
    RootObj->SetNumberField(TEXT("temperature"), 0.3);
    RootObj->SetNumberField(TEXT("max_tokens"), 300);

    FString Body;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
    FJsonSerializer::Serialize(RootObj.ToSharedRef(), Writer);
    HttpRequest->SetContentAsString(Body);

    FLLMRequest RequestCopy = Request;

    GetWorld()->GetTimerManager().SetTimer(TimeoutTimerHandle,
        [this, RequestCopy]()
        {
            UE_LOG(LogTemp, Error,
                TEXT("[LLMQueue][#%d] 타임아웃(%.0f초) — 폴백 처리"),
                RequestCopy.RequestID, TimeoutSeconds);

            ApplyFallback(RequestCopy);
            ProcessNext();
        }, TimeoutSeconds, false);

    HttpRequest->OnProcessRequestComplete().BindLambda(
        [this, RequestCopy, SendTime](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            GetWorld()->GetTimerManager().ClearTimer(TimeoutTimerHandle);

            // 추가 — 걸린 시간 계산
            float ElapsedTime = GetWorld()->GetTimeSeconds() - SendTime;

            UE_LOG(LogTemp, Warning,
                TEXT("[LLMQueue][#%d] 콜백 도착 — bSuccess:%d, RespValid:%d, 소요:%.2f초"),
                RequestCopy.RequestID, bSuccess, Resp.IsValid(), ElapsedTime);

            if (!bSuccess || !Resp.IsValid())
            {
                if (Resp.IsValid())
                {
                    UE_LOG(LogTemp, Error,
                        TEXT("[LLMQueue][#%d] HTTP코드:%d 본문:%s"),
                        RequestCopy.RequestID, Resp->GetResponseCode(),
                        *Resp->GetContentAsString());
                }
                else
                {
                    UE_LOG(LogTemp, Error,
                        TEXT("[LLMQueue][#%d] Resp 자체가 무효"), RequestCopy.RequestID);
                }

                ApplyFallback(RequestCopy);
                ProcessNext();
                return;
            }

            UE_LOG(LogTemp, Warning,
                TEXT("[LLMQueue][#%d] 응답 정상 — 길이:%d"),
                RequestCopy.RequestID, Resp->GetContentAsString().Len());

            if (RequestCopy.RequestType == ELLMRequestType::GroupCommand)
            {
                if (RequestCopy.TargetGroup.IsValid())
                {
                    ParseAndApplyGroupResponse(
                        Resp->GetContentAsString(), RequestCopy.TargetGroup.Get());
                }
                else
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("[LLMQueue][#%d] 응답 도착했으나 그룹 이미 무효"),
                        RequestCopy.RequestID);
                }
            }
            else
            {
                ParseAndApplyResponse(
                    Resp->GetContentAsString(),
                    RequestCopy.TargetPersonality.Get(),
                    RequestCopy.TargetActor.Get());
            }

            ProcessNext();
        });

    bool bStarted = HttpRequest->ProcessRequest();
    UE_LOG(LogTemp, Warning,
        TEXT("[LLMQueue][#%d] ProcessRequest 시작 성공: %d"),
        Request.RequestID, bStarted);
}

void ULLMRequestQueue::OnResponseReceived(FHttpRequestPtr Req, FHttpResponsePtr Resp,
    bool bSuccess, FLLMRequest Request)
{
    GetWorld()->GetTimerManager().ClearTimer(TimeoutTimerHandle);

    if (!bSuccess || !Resp.IsValid() || !Request.TargetPersonality.IsValid())
    {
        ApplyFallback(Request);
        ProcessNext();
        return;
    }

    FString ResponseStr = Resp->GetContentAsString();
    UE_LOG(LogTemp, Warning, TEXT("[LLMQueue] 응답 수신: %s"), *ResponseStr);

    ParseAndApplyResponse(
        Resp->GetContentAsString(),
        Request.TargetPersonality.Get(),
        Request.TargetActor.Get()
    );

    ProcessNext();
}

void ULLMRequestQueue::ParseAndApplyResponse(
    const FString& ResponseJson, UPersonalityComponent* Personality, AActor* TargetActor)
{
    if (!Personality) return;
    if (!IsValid(TargetActor)) return;

    if (UHealthComponent* HC = TargetActor->FindComponentByClass<UHealthComponent>())
    {
        if (HC->IsDead())
        {
            UE_LOG(LogTemp, Warning, TEXT("[LLM] 대상 이미 사망 — 처리 스킵"));
            return;
        }
    }

    // 1. 바깥 래퍼 파싱 (choices[0].message.content)
    TSharedPtr<FJsonObject> RootObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseJson);
    if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] 응답 파싱 실패 (바깥)"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* Choices;
    if (!RootObj->TryGetArrayField(TEXT("choices"), Choices) || Choices->Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] choices 없음"));
        return;
    }

    FString Content = (*Choices)[0]->AsObject()
        ->GetObjectField(TEXT("message"))->GetStringField(TEXT("content"));

    // 2. 마크다운 코드블록 제거
    Content = Content.Replace(TEXT("```json"), TEXT(""));
    Content = Content.Replace(TEXT("```"), TEXT(""));
    Content = Content.TrimStartAndEnd();

    // 3. 내부 JSON(성격 델타) 파싱
    TSharedPtr<FJsonObject> DeltaObj;
    TSharedRef<TJsonReader<>> DeltaReader = TJsonReaderFactory<>::Create(Content);
    if (!FJsonSerializer::Deserialize(DeltaReader, DeltaObj) || !DeltaObj.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[LLM] 델타 JSON 파싱 실패: %s"), *Content);
        return;
    }

    FPersonalityDelta Delta;
    DeltaObj->TryGetNumberField(TEXT("Aggression"), Delta.Aggression);
    DeltaObj->TryGetNumberField(TEXT("Fear"), Delta.Fear);
    DeltaObj->TryGetNumberField(TEXT("Trust"), Delta.Trust);
    DeltaObj->TryGetNumberField(TEXT("Courage"), Delta.Courage);
    DeltaObj->TryGetNumberField(TEXT("FearSensitivity"), Delta.FearSensitivity);

    Personality->ApplyDelta(Delta);

    // 대사 추출 — 실패해도 성격 적용에는 영향 없음
    FString Dialogue;
    if (DeltaObj->TryGetStringField(TEXT("Dialogue"), Dialogue) && !Dialogue.IsEmpty())
    {
        if (IsValid(TargetActor))
        {
            if (UDialogComponent* Dialog =
                TargetActor->FindComponentByClass<UDialogComponent>())
            {
                Dialog->ShowDialogue(Dialogue);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[LLM] Dialogue 필드 없음 — 대사 스킵"));
    }

    UE_LOG(LogTemp, Warning, TEXT("[LLM] 성격 델타 적용 완료"));
}

void ULLMRequestQueue::ParseAndApplyGroupResponse(
    const FString& ResponseJson, AGroupManagerActor* Group)
{
    if (!IsValid(Group)) return;

    TSharedPtr<FJsonObject> RootObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseJson);
    // if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid()) return;
    if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[GroupLLM] 바깥 JSON 파싱 실패: %s"), *ResponseJson);
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* Choices;
    // if (!RootObj->TryGetArrayField(TEXT("choices"), Choices) || Choices->Num() == 0) return;
    if (!RootObj->TryGetArrayField(TEXT("choices"), Choices) || Choices->Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[GroupLLM] choices 필드 없음"));
        return;
    }
    
    FString Content = (*Choices)[0]->AsObject()
        ->GetObjectField(TEXT("message"))->GetStringField(TEXT("content"));
    Content = Content.Replace(TEXT("```json"), TEXT("")).Replace(TEXT("```"), TEXT(""));
    Content = Content.TrimStartAndEnd();

    TSharedPtr<FJsonObject> BodyObj;
    TSharedRef<TJsonReader<>> BodyReader = TJsonReaderFactory<>::Create(Content);
    // if (!FJsonSerializer::Deserialize(BodyReader, BodyObj) || !BodyObj.IsValid()) return;
    if (!FJsonSerializer::Deserialize(BodyReader, BodyObj) || !BodyObj.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[GroupLLM] 내부 JSON 파싱 실패: %s"), *Content);
        return;
    }
    
    // 멤버별 델타 — ID 매칭으로 분배
    const TArray<TSharedPtr<FJsonValue>>* MembersArray;
    if (BodyObj->TryGetArrayField(TEXT("Members"), MembersArray))
    {
        for (const auto& MemberVal : *MembersArray)
        {
            TSharedPtr<FJsonObject> MemberObj = MemberVal->AsObject();
            FString ID = MemberObj->GetStringField(TEXT("ID"));

            bool bMatched = false; // debug
            for (TObjectPtr<ARetryNPCCharacter> Member : Group->Members)
            {
                if (!IsValid(Member) || Member->GetName() != ID) continue;
                bMatched = true; // debug

                UPersonalityComponent* PC =
                    Member->FindComponentByClass<UPersonalityComponent>();
                if (!PC) continue;

                FPersonalityDelta Delta;
                MemberObj->TryGetNumberField(TEXT("Aggression"), Delta.Aggression);
                MemberObj->TryGetNumberField(TEXT("Fear"), Delta.Fear);
                MemberObj->TryGetNumberField(TEXT("Trust"), Delta.Trust);
                MemberObj->TryGetNumberField(TEXT("Courage"), Delta.Courage);
                MemberObj->TryGetNumberField(TEXT("Loyalty"), Delta.Loyalty);

                PC->ApplyDelta(Delta);
                break;
            }

            // debug
            if (!bMatched)
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("[GroupLLM] ID 매칭 실패 — 응답ID: %s"), *ID);
            }
        }
    }
    else //debug
    {
        UE_LOG(LogTemp, Error, TEXT("[GroupLLM] Members 필드 없음: %s"), *Content);
    }

    // Order 필드 파싱 → 그룹 전체에 전파
    FString OrderStr;
    if (BodyObj->TryGetStringField(TEXT("Order"), OrderStr))
    {
        ENPCOrder Order = ENPCOrder::HoldFire;
        if (OrderStr == TEXT("FreeFire"))  Order = ENPCOrder::FreeFire;
        else if (OrderStr == TEXT("Charge"))    Order = ENPCOrder::Charge;
        else if (OrderStr == TEXT("Retreat"))   Order = ENPCOrder::Retreat;
        else if (OrderStr == TEXT("TakeCover")) Order = ENPCOrder::TakeCover;

        Group->SetOrderForAll(Order, 0.5f);  // 기본 가중치 0.5
    }
    else //debug
    {
        UE_LOG(LogTemp, Warning, TEXT("[GroupLLM] Order 필드 없음"));
    }
}

void ULLMRequestQueue::ApplyFallback(FLLMRequest Request)
{
    UE_LOG(LogTemp, Warning, TEXT("[LLMQueue] 폴백 — 성격 변화 없음"));

    if (Request.TargetActor.IsValid() && Request.TargetPersonality.IsValid())
    {
        PlayFallbackDialogue(Request.TargetActor.Get(),
            Request.TargetPersonality.Get());
    }
}

void ULLMRequestQueue::PlayFallbackDialogue(AActor* TargetActor, UPersonalityComponent* Personality)
{
    if (!FallbackDialogueTable || !Personality) return;
    if (!IsValid(TargetActor)) return;

    if (UHealthComponent* HC =
        TargetActor->FindComponentByClass<UHealthComponent>())
    {
        if (HC->IsDead()) return;
    }

    EPersonalityType MyType = Personality->GetPersonalityType();

    TArray<FFallbackDialogueRow*> Matched;
    TArray<FName> RowNames = FallbackDialogueTable->GetRowNames();

    for (const FName& RowName : RowNames)
    {
        FFallbackDialogueRow* Row =
            FallbackDialogueTable->FindRow<FFallbackDialogueRow>(
                RowName, TEXT("FallbackDialogue"));

        if (Row && Row->PersonalityType == MyType)
        {
            Matched.Add(Row);
        }
    }

    if (Matched.Num() == 0) return;

    int32 RandomIndex = FMath::RandRange(0, Matched.Num() - 1);
    FString ChosenText = Matched[RandomIndex]->DialogueText;

    if (UDialogComponent* Dialog =
        TargetActor->FindComponentByClass<UDialogComponent>())
    {
        Dialog->ShowDialogue(ChosenText);
    }
}

void ULLMRequestQueue::EnqueueGroupRequest(AGroupManagerActor* Group)
{
    if (!IsValid(Group)) return;

    FLLMRequest Request;
    Request.RequestType = ELLMRequestType::GroupCommand;
    Request.TargetGroup = Group;
    Request.Prompt = Group->BuildGroupLLMPrompt();
    Request.RequestTime = GetWorld()->GetTimeSeconds();

    Enqueue(Request);
}
