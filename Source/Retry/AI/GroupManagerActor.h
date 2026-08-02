#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPCOrderTypes.h"
#include "GroupMemoryTypes.h"
#include "GroupManagerActor.generated.h"

UCLASS()
class RETRY_API AGroupManagerActor : public AActor
{
	GENERATED_BODY()

public:
	AGroupManagerActor();

	UPROPERTY(EditAnywhere, Category="Group")
	FString GroupID;

	UPROPERTY()
	class ARetryNPCCharacter* Leader;

	UPROPERTY()
	TArray<class ARetryNPCCharacter*> Members;

	UFUNCTION(BlueprintCallable, Category="Group")
	void RegisterMember(ARetryNPCCharacter* NPC, bool bIsLeader);

	UFUNCTION(BlueprintCallable, Category="Group")
	void AddGroupMemory(const FString& WitnessID, const FString& EventType,
		FVector Location, float EmotionWeight, const FString& Description);

	UFUNCTION(BlueprintPure, Category="Group")
	TArray<FGroupMemoryEvent> GetRecentGroupMemories(int32 Count) const;

	UFUNCTION(BlueprintCallable, Category="Group")
	void OnLeaderDied();   // 리더의 OnDeath에서 호출

	UFUNCTION(BlueprintCallable, Category="Group")
	void SetOrderForAll(ENPCOrder Order, float Weight);

	UPROPERTY(EditAnywhere, Category="Group")
	float GroupEmotionThreshold = 0.5f;

	FString BuildGroupLLMPrompt() const;

private:
	TArray<FGroupMemoryEvent> GroupMemories;
	float AccumulatedEmotionScore = 0.f;
};