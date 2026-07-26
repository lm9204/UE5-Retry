// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_SearchArea.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_SearchArea::UBTTask_SearchArea()
{
	NodeName = TEXT("Search Area");
}

EBTNodeResult::Type UBTTask_SearchArea::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	FVector SearchCenter = BB->GetValueAsVector(TEXT("LastKnownEnemyLocation"));

	UNavigationSystemV1* NavSys =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) return EBTNodeResult::Failed;

	// 수색 포인트 여러 개 생성
	TArray<FVector> SearchPoints;
	for (int32 i = 0; i < FMath::FloorToInt(SearchDuration / SearchPointPerSecond); ++i)
	{
		FNavLocation NavLoc;
		if (NavSys->GetRandomReachablePointInRadius(
			SearchCenter, SearchRadius, NavLoc))
		{
			SearchPoints.Add(NavLoc.Location);
		}
	}

	if (SearchPoints.Num() == 0) return EBTNodeResult::Failed;

	// 타이머로 순서대로 이동
	for (int32 i = 0; i < SearchPoints.Num(); ++i)
	{
		FVector Point = SearchPoints[i];
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, [AIC, Point]()
		{
			if (IsValid(AIC))
				AIC->MoveToLocation(Point, 50.f);
		}, SearchPointPerSecond * i, false);
	}

	return EBTNodeResult::Succeeded;
}
