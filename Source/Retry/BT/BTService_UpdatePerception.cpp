// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTService_UpdatePerception.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_UpdatePerception::UBTService_UpdatePerception()
{
	NodeName = TEXT("Update Perception");
	Interval = 0.1f;
	RandomDeviation = 0.02f;
}

void UBTService_UpdatePerception::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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
	if (!Target)
	{
		BB->SetValueAsBool(TEXT("bCanSeeTarget"), false);
		return;
	}

	// 시야 체크 - LineTrace로 시야 차단 여부 확인
	FHitResult HitResult;
	FVector Start = NPC->GetActorLocation() + FVector(0, 0, 60.f);
	FVector End = Target->GetActorLocation() + FVector(0, 0, 60.f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(NPC);
	Params.AddIgnoredActor(Target);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult, Start, End, ECC_Visibility, Params);

	float Dist = FVector::Dist(NPC->GetActorLocation(), Target->GetActorLocation());

	// 시야 내 + 거리 내 + 시야 차단 없음
	bool bCanSee = !bHit && Dist <= SightRange;
	BB->SetValueAsBool(TEXT("bCanSeeTarget"), bCanSee);

	if (bCanSee)
	{
		BB->SetValueAsBool(TEXT("bAlerted"), true);
	}
}
