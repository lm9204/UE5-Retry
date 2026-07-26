// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_CallOut.h"

#include "AIController.h"
#include "RetryNPCCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

UBTTask_CallOut::UBTTask_CallOut()
{
	NodeName = TEXT("Call Out");
}

EBTNodeResult::Type UBTTask_CallOut::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	APawn* NPC = AIC->GetPawn();
	if (!NPC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	FVector EnemyLoc = BB->GetValueAsVector(
		TEXT("LastKnownEnemyLocation"));

	// 주변 NPC들에게 적 위치 전파
	TArray<AActor*> NearbyNPCs;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), ARetryNPCCharacter::StaticClass(), NearbyNPCs);

	for (AActor* Actor : NearbyNPCs)
	{
		if (Actor == NPC) continue;

		float Dist = FVector::Dist(
			NPC->GetActorLocation(), Actor->GetActorLocation());
		if (Dist > AlertRadius) continue;

		// 주변 NPC BLackboard에 적 위치 전파
		if (ARetryNPCCharacter* NearNPC = Cast<ARetryNPCCharacter>(Actor))
		{
			if (AAIController* NearAIC = Cast<AAIController>(NearNPC->GetController()))
			{
				if (UBlackboardComponent* NearBB = NearAIC->GetBlackboardComponent())
				{
					NearBB->SetValueAsVector(TEXT("StimulusLocation"), EnemyLoc);
					NearBB->SetValueAsBool(TEXT("bAlerted"), true);
				}
			}
		}
	}

	return EBTNodeResult::Succeeded;
}
