// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_Overwatch.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_Overwatch::UBTTask_Overwatch()
{
	NodeName = TEXT("Overwatch");
	bNotifyTick = false;
}

EBTNodeResult::Type UBTTask_Overwatch::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	// 자극 방향으로 회전
	FVector StimulusLoc = BB->GetValueAsVector(
		TEXT("StimulusLocation"));

	if (!StimulusLoc.IsZero())
		AIC->SetFocalPoint(StimulusLoc);

	// WatchDuration 후 Alert 해제
	// FTimerHandle TimerHandle;
	// UBehaviorTreeComponent* BTComp = &OwnerComp;
	// GetWorld()->GetTimerManager().SetTimer(TimerHandle, [BB, BTComp]()
	// {
	// 	if (BB) BB->SetValueAsBool(TEXT("bAlerted"), false);
	// }, WatchDuration, false);

	return EBTNodeResult::Succeeded;
}
