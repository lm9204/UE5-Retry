// Fill out your copyright notice in the Description page of Project Settings.


#include "RetryNPCController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Navigation/CrowdFollowingComponent.h"

ARetryNPCController::ARetryNPCController(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(
		TEXT("PathFollowingComponent")))
{
}

void ARetryNPCController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
		UE_LOG(LogTemp, Warning, TEXT("[NPC Controller] Behavior Tree 실행: %s"),
			*BehaviorTreeAsset->GetName());
	}
}


