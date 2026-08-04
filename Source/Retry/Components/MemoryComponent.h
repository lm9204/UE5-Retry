#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MemoryComponent.generated.h"

UENUM()
enum class EMemoryEventType : uint8
{
	AllyDeath,
	EnemyKilled,
	CombatDefeat,
	PlayerCommand,
	ValuableItemFound,
};

USTRUCT(BlueprintType)
struct FNPCMemory
{
	GENERATED_BODY()

	UPROPERTY()
	EMemoryEventType EventType = EMemoryEventType::AllyDeath;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	float Timestamp = 0.f;

	UPROPERTY()
	float EmotionWeight = 0.f;

	UPROPERTY()
	FString Description;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMemoryThreshold);

UCLASS( ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RETRY_API UMemoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMemoryComponent();

	UFUNCTION(BlueprintCallable, Category="Memory")
	void AddMemory(EMemoryEventType EventType, FVector Location,
		float EmotionWeight, const FString& Description);

	UFUNCTION(BlueprintPure, Category="Memory")
	TArray<FNPCMemory> GetRecentMemories(int32 Count) const;

	UFUNCTION(BlueprintCallable, Category="Memory")
	void ResetMemories();

	UPROPERTY(BlueprintAssignable, Category="Memory")
	FOnMemoryThreshold OnMemoryThreshold;

	UPROPERTY(EditAnywhere, Category="Memory")
	float EmotionScoreThreshold = 0.5f;	

	UPROPERTY(EditAnywhere, Category="Memory")
	int32 MaxMemoryCount = 20;
	
private:
	UPROPERTY()
	TArray<FNPCMemory> Memories;

	float AccumulatedEmotionScore = 0.f;
};
