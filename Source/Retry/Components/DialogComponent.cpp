#include "DialogComponent.h"

UDialogComponent::UDialogComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDialogComponent::ShowDialogue(const FString& Text, float Duration)
{
	// 연속 호출 시 기존 타이머 취소 (겹침 방지)
	GetWorld()->GetTimerManager().ClearTimer(DialogueTimerHandle);

	OnDialogueRequested.Broadcast(Text, Duration);

	UE_LOG(LogTemp, Warning, TEXT("[Dialogue] %s: %s"),
		*GetOwner()->GetName(), *Text);
}