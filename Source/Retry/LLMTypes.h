#pragma once

#include "CoreMinimal.h"
#include "LLMTypes.generated.h"

UENUM(BlueprintType)
enum class ELLMRequestType : uint8
{
	MemoryEvaluation, // 메모리 누적 -> 성격 재평가
	GroupCommand,
	// @TODO Dialogue, TacticalAdvice, etc..
};

USTRUCT(BlueprintType)
struct FLLMRequest
{
	GENERATED_BODY()

	TWeakObjectPtr<class UPersonalityComponent> TargetPersonality;
	TWeakObjectPtr<class AActor> TargetActor;
	TWeakObjectPtr<class AGroupManagerActor> TargetGroup;

	ELLMRequestType RequestType = ELLMRequestType::MemoryEvaluation;
	FString Prompt;

	float RequestTime = 0.f;

	int32 RequestID = -1;
};

