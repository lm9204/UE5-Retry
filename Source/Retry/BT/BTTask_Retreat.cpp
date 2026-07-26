// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_Retreat.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_Retreat::UBTTask_Retreat()
{
	NodeName = TEXT("Retreat");
}

EBTNodeResult::Type UBTTask_Retreat::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	APawn* NPC = AIC->GetPawn();
	if (!NPC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* Target = Cast<AActor>(
		BB->GetValueAsObject(TEXT("TargetActor")));
	if (!Target) return EBTNodeResult::Failed;

	// 적 반대 방향으로 후퇴 위치 탐색
	FVector AwayDir = (NPC->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
	FVector RetreatPos = NPC->GetActorLocation() + AwayDir * RetreatDistance;

	UNavigationSystemV1* NavSys =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) return EBTNodeResult::Failed;

	FNavLocation NavLoc;
	if (NavSys->GetRandomReachablePointInRadius(RetreatPos, 300.f, NavLoc))
	{
		AIC->MoveToLocation(NavLoc.Location, 50.f);
		return EBTNodeResult::Succeeded;
	}
	return EBTNodeResult::Failed;
}
