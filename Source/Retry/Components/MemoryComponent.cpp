#include "Components/MemoryComponent.h"
#include "Debug/CombatLogging.h"

UMemoryComponent::UMemoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMemoryComponent::AddMemory(EMemoryEventType EventType, FVector Location, float EmotionWeight,
	const FString& Description)
{
	FNPCMemory Memory;
	Memory.EventType = EventType;
	Memory.Location = Location;
	Memory.Timestamp = GetWorld()->GetTimeSeconds();
	Memory.EmotionWeight = EmotionWeight;
	Memory.Description = Description;

	Memories.Add(Memory);
	if (Memories.Num() > MaxMemoryCount)
			Memories.RemoveAt(0);

	AccumulatedEmotionScore += EmotionWeight;

	UE_LOG(LogTemp, Warning, TEXT("[Memory] %s: %s 기록 (Score: %.2f/%.2f)"),
		*GetCombatLogName(GetOwner()), *Description, AccumulatedEmotionScore, EmotionScoreThreshold);

	if (AccumulatedEmotionScore >= EmotionScoreThreshold)
	{
		OnMemoryThreshold.Broadcast();
		AccumulatedEmotionScore = 0.f;
	}
}

TArray<FNPCMemory> UMemoryComponent::GetRecentMemories(int32 Count) const
{
	TArray<FNPCMemory> Result;
	int32 Start = FMath::Max(0, Memories.Num() - Count);
	for (int32 i = Start; i < Memories.Num(); i++)
		Result.Add(Memories[i]);
	return Result;
}

void UMemoryComponent::ResetMemories()
{
	Memories.Reset();
	AccumulatedEmotionScore = 0.f;
}
