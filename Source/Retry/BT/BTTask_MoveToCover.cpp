// Fill out your copyright notice in the Description page of Project Settings.


#include "BT/BTTask_MoveToCover.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Debug/CombatLogging.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_MoveToCover::UBTTask_MoveToCover()
{
	NodeName=TEXT("Move To Cover");
}

EBTNodeResult::Type UBTTask_MoveToCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	FVector CoverLoc = BB->GetValueAsVector(TEXT("CoverLocation"));
	UE_LOG(LogTemp, Warning, TEXT("[MoveToCover] %s CoverLoc: %s"),
		*GetCombatLogName(AIC->GetPawn()), *CoverLoc.ToString());

	if (CoverLoc.IsZero()) return EBTNodeResult::Failed;
	
	FAIMoveRequest MoveReq(CoverLoc);
	MoveReq.SetAcceptanceRadius(50.f);

	FPathFollowingRequestResult Result = AIC->MoveTo(MoveReq);

	if (Result.Code == EPathFollowingRequestResult::Failed)
		return EBTNodeResult::Failed;

	if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
		return EBTNodeResult::Succeeded;

	// 이동 중이면 InProgress — MoveTo 완료 시 OnMoveCompleted에서 처리 필요
	return EBTNodeResult::InProgress;
}
