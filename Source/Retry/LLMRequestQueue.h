// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LLMTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "LLMRequestQueue.generated.h"

/**
 * 
 */
UCLASS()
class RETRY_API ULLMRequestQueue : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LLM")
	void Enqueue(const FLLMRequest& Request);

	UPROPERTY(EditAnywhere, Category="LLM")
	FString ServerURL = TEXT("http://localhost:8080/v1/chat/completions");

	UPROPERTY(EditAnywhere, Category="LLM")
	float TimeoutSeconds = 3.f;	
		
private:
	TQueue<FLLMRequest> PendingRequests;
	bool bIsProcessing = false;

	void ProcessNext();
	void SendRequest(const FLLMRequest& Request);

	void OnResponseReceived(FHttpRequestPtr Req, FHttpResponsePtr Resp,
		bool bSuceess, FLLMRequest Request);
	void ParseAndApplyResponse(const FString& ResponseJson,
		UPersonalityComponent* Personality);

	void ApplyFallback(FLLMRequest Request);

	FTimerHandle TimeoutTimerHandle;
};
