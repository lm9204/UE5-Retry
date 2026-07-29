#include "LLMRequestQueue.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "Components/PersonalityComponent.h"
#include "Engine/World.h"

void ULLMRequestQueue::Enqueue(const FLLMRequest& Request)
{
    PendingRequests.Enqueue(Request);
    UE_LOG(LogTemp, Warning, TEXT("[LLMQueue] 요청 추가됨"));

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
    SendRequest(Request);
}

void ULLMRequestQueue::SendRequest(const FLLMRequest& Request)
{
    if (!Request.TargetPersonality.IsValid())
    {
        ProcessNext();  // 대상 사라졌으면 스킵
        return;
    }

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

    FString Body;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
    FJsonSerializer::Serialize(RootObj.ToSharedRef(), Writer);
    HttpRequest->SetContentAsString(Body);

    HttpRequest->OnProcessRequestComplete().BindUObject(
        this, &ULLMRequestQueue::OnResponseReceived, Request);

    // 타임아웃 타이머
    FLLMRequest RequestCopy = Request;
    GetWorld()->GetTimerManager().SetTimer(TimeoutTimerHandle, [this, RequestCopy]()
    {
        UE_LOG(LogTemp, Warning, TEXT("[LLMQueue] 타임아웃 — 폴백 처리"));
        ApplyFallback(RequestCopy);
        ProcessNext();
    }, TimeoutSeconds, false);

    HttpRequest->ProcessRequest();
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

    ParseAndApplyResponse(Resp->GetContentAsString(), Request.TargetPersonality.Get());

    ProcessNext();
}

void ULLMRequestQueue::ParseAndApplyResponse(
    const FString& ResponseJson, UPersonalityComponent* Personality)
{
    if (!Personality) return;

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

    UE_LOG(LogTemp, Warning, TEXT("[LLM] 성격 델타 적용 완료"));
}

void ULLMRequestQueue::ApplyFallback(FLLMRequest Request)
{
    // 폴백 — 성격 변화 없음, 그냥 로그만
    UE_LOG(LogTemp, Warning, TEXT("[LLMQueue] 폴백 — 성격 변화 없음"));
}