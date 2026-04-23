// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_SelectCombatRoute.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/PersonalityComponent.h"

UBTTask_SelectCombatRoute::UBTTask_SelectCombatRoute()
{
	NodeName  = TEXT("Select Combat Route");
}

EBTNodeResult::Type UBTTask_SelectCombatRoute::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	APawn* NPC = AIC->GetPawn();
	if (!NPC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));

	UPersonalityComponent* PC =
		NPC->FindComponentByClass<UPersonalityComponent>();
	if (!PC) return EBTNodeResult::Failed;

	float Aggression = PC->GetAggression();
	float Fear = PC->GetFear();
	float Trust = PC->GetTrust();

	FVector DestLocation = NPC->GetActorLocation();

	if (Aggression > 0.7f && Target)
	{
		DestLocation = GetAggressiveRoute(Target, NPC);
	}
	else if (Fear > 0.6f && Target)
	{
		DestLocation = GetCautiousRoute(Target, NPC);
	}
	else if (Trust > 0.7f)
	{
		DestLocation = GetSupportiveRoute(NPC);
	}
	else
	{
		DestLocation = GetOpportunistRoute(Target, NPC, Aggression, Fear);
	}

	BB->SetValueAsVector(TEXT("MoveDestination"), DestLocation);

	return EBTNodeResult::Succeeded;
}

FVector UBTTask_SelectCombatRoute::GetAggressiveRoute(AActor* Target, APawn* NPC)
{
	return Target->GetActorLocation();
}

FVector UBTTask_SelectCombatRoute::GetCautiousRoute(AActor* Target, APawn* NPC)
{
	FVector AwayDir = (NPC->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
	FVector SafeLocation = NPC->GetActorLocation() + AwayDir * 500.f;
	
	FNavLocation NavLoc;
	UNavigationSystemV1* NavSys =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(NPC->GetWorld());
	
	if (NavSys && NavSys->GetRandomReachablePointInRadius(
		SafeLocation, 200.f, NavLoc))
	{
		return NavLoc.Location;
	}

	return SafeLocation;
}

FVector UBTTask_SelectCombatRoute::GetSupportiveRoute(APawn* NPC)
{
	return NPC->GetActorLocation();
}

FVector UBTTask_SelectCombatRoute::GetOpportunistRoute(AActor* Target, APawn* NPC, float Aggression, float Fear)
{
	if (!Target) return NPC->GetActorLocation();

	if (Aggression > Fear)
	{
		FVector ToTarget = (Target->GetActorLocation() - NPC->GetActorLocation()).GetSafeNormal();
		return NPC->GetActorLocation() + ToTarget * 300.f;
	}
	else
	{
		FVector AwayDir = (NPC->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
		return  NPC->GetActorLocation() + AwayDir * 400.f;
	}
}
