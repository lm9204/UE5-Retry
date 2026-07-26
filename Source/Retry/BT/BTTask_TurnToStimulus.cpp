// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_TurnToStimulus.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_TurnToStimulus::UBTTask_TurnToStimulus()
{
	NodeName = TEXT("Turn To Stimulus");
}

EBTNodeResult::Type UBTTask_TurnToStimulus::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	FVector StimulusLoc = BB->GetValueAsVector(TEXT("StimulusLocation"));

	if (StimulusLoc.IsZero()) return EBTNodeResult::Failed;

	AIC->SetFocalPoint(StimulusLoc);
	return EBTNodeResult::Succeeded;
}
