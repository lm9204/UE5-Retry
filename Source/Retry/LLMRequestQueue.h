// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LLMTypes.h"
#include "AI/GroupManagerActor.h"
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
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="LLM")
	void Enqueue(const FLLMRequest& Request);

	UFUNCTION(BlueprintCallable, Category="LLM")
	void EnqueueGroupRequest(class AGroupManagerActor* Group);

	UFUNCTION(BlueprintCallable, Category="LLM")
	void ResetQueueForScenarioTransition();

	UFUNCTION(BlueprintPure, Category="LLM")
	bool IsRequestEnabledForCurrentContext() const;

	UPROPERTY(EditAnywhere, Category="LLM")
	FString ServerURL = TEXT("http://localhost:8080/v1/chat/completions");

	UPROPERTY(EditAnywhere, Category="LLM")
	float TimeoutSeconds = 40.f;
		
private:
	TQueue<FLLMRequest> PendingRequests;
	bool bIsProcessing = false;
	FHttpRequestPtr ActiveRequest;
	uint64 RequestGeneration = 0;

	int32 NextRequestID = 0;

	void ProcessNext();
	void SendRequest(const FLLMRequest& Request);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void ParseAndApplyResponse(const FString& ResponseJson,
		UPersonalityComponent* Personality, AActor* TargetActor);
	void ParseAndApplyGroupResponse(const FString& ResponseJson,
		AGroupManagerActor* Group);

	void ApplyFallback(FLLMRequest Request);

	void PlayFallbackDialogue(AActor* TargetActor, UPersonalityComponent* Personality);

	UPROPERTY(EditAnywhere, Category="Dialogue")
	class UDataTable* FallbackDialogueTable;
};
