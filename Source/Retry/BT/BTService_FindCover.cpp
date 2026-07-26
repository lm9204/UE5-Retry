// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTService_FindCover.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_FindCover::UBTService_FindCover()
{
	NodeName = TEXT("Find Cover");
	Interval = 0.5f;
	RandomDeviation = 0.1f;
}

void UBTService_FindCover::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return;

	APawn* NPC = AIC->GetPawn();
	if (!NPC) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	AActor* Target = Cast<AActor>(
		BB->GetValueAsObject(TEXT("TargetActor")));
	if (!Target) return;

	UNavigationSystemV1* NavSys =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) return;

	FVector BestCover = FVector::ZeroVector;
	float BestScore = -1.f;

	for (int32 i = 0; i < NumTestPoints; ++i)
	{
		FNavLocation NavLoc;
		if (!NavSys->GetRandomReachablePointInRadius(
			NPC->GetActorLocation(), SearchRadius, NavLoc))
			continue;

		FVector TestPoint = NavLoc.Location;

		// 적에서 이 포인트까지 시야 차단되는지 체크
		if (!IsInCover(TestPoint, Target->GetActorLocation(), NPC))
			continue;

		// 점수 계산 - 적당한 거리에 있을수록 높음
		float DistToEnemy = FVector::Dist(
			TestPoint, Target->GetActorLocation());
		float DistToSelf = FVector::Dist(
			TestPoint, NPC->GetActorLocation());

		// 너무 가깝거나 멀면 점수 낮음
		float Score = FMath::Clamp(DistToEnemy, 300.f, 800.f)
		/ 800.f - DistToSelf / SearchRadius;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestCover = TestPoint;
		}
	}

	if (BestScore > 0.f)
	{
		BB->SetValueAsVector(TEXT("CoverLocation"), BestCover);
	}
}

bool UBTService_FindCover::IsInCover(FVector TestPoint, FVector EnemyLocation, APawn* NPC)
{
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(NPC);

	// 적에서 테스트 포인트까지 뭔가 막히면 엄폐 가능
	return GetWorld()->LineTraceSingleByChannel(
		HitResult,
		EnemyLocation + FVector(0, 0, 60.f),
		TestPoint + FVector(0, 0, 60.f),
		ECC_Visibility,
		Params);
}
