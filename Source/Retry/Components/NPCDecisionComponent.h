// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPCContext.h"
#include "NPCDecisionComponent.generated.h"


UCLASS( ClassGroup=(AI), meta=(BlueprintSpawnableComponent) )
class RETRY_API UNPCDecisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UNPCDecisionComponent();

	// BTService가 호출하는 진입점
	void Update(float DeltaSeconds);

	ENPCCombatState GetCombatState() const { return CurrentState; }

	// 상태 안정화
	UPROPERTY(EditAnywhere, Category="Decision")
	float StateMinDuration = 1.0f;

	// 점수 가중치
	UPROPERTY(EditAnywhere, Category="Scoring")
	float AttackScoreWeight = 1.0f;
	
	UPROPERTY(EditAnywhere, Category="Scoring")
	float CoverScoreWeight = 1.0f;
	
	UPROPERTY(EditAnywhere, Category="Scoring")
	float RetreatScoreWeight = 1.0f;

	UPROPERTY(EditAnywhere, Category="Scoring")
	float ReloadScoreWeight = 1.0f;

	// Perception 설정
	// AAIController의 SightConfig->LoseSightRadius와 맞춰야 한다 — Perception은
	// 한 번 감지한 타겟을 LoseSightRadius까지는 계속 "인지 중"으로 유지하는데
	// (히스테리시스), 이 값이 그보다 작으면 Perception은 타겟을 여전히 인지 중이라
	// CombatState가 Attack/TakeCover를 유지하면서도 CheckLineOfSight의 거리
	// 컷오프에 걸려 영원히 사격하지 못하는 사각지대가 생긴다.
	UPROPERTY(EditAnywhere, Category="Perception")
	float SightRange = 3000.f;

	UPROPERTY(EditAnywhere, Category="Perception")
	float LostSightTimeout = 3.f;

	// Cover 탐색 설정
	UPROPERTY(EditAnywhere, Category="Cover")
	float CoverSearchRadius = 800.f;
	
	UPROPERTY(EditAnywhere, Category="Cover")
	int32 CoverTestPoints = 16;
	
	UPROPERTY(EditAnywhere, Category="Cover")
	float CoverSearchInterval = 0.5f;

	// 디버그
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bDebugEnabled = false;

	FNPCScoreSnapshot LastScoreSnapshot;
	TArray<FNPCStateTransition> TransitionHistory;

private:
	// Update 내부 단계
	void UpdateAIState(float DeltaSeconds);
	void UpdateCombatState();
	void UpdateDialogueState();
	void WriteBlackboard();
	void UpdateDebugWidget();

	// Context 수집
	FNPCContext CollectContext();

	// 판단
	ENPCCombatState DecideState(const FNPCContext& Ctx);
	ENPCCombatState GetStableState(ENPCCombatState Candidate,
									const FNPCContext& Ctx);	
	bool ShouldForceExit(ENPCCombatState State,
									const FNPCContext& Ctx);

	// 점수 계산
	float CalcAttackScore(const FNPCContext& Ctx);
	float CalcCoverScore(const FNPCContext& Ctx);
	float CalcRetreatScore(const FNPCContext& Ctx);
	float CalcReloadScore(const FNPCContext& Ctx);

	// 유틸
	bool CheckLineOfSight(APawn* NPC, AActor* Target);
	void FindBestCoverLocation(APawn* NPC, AActor* Target, FNPCContext& Ctx);
	void RecordTransition(ENPCCombatState From,
							ENPCCombatState To,
							const FString& Reason);

	// 내부 상태
	FNPCContext		CachedContext;
	ENPCCombatState	CurrentState = ENPCCombatState::Idle;
	float			TimeSinceChange = 0.f;
	float			TimeSinceLostSight = 0.f;
	float			TimeSinceCoverSearch = 0.f;

	static constexpr int32 MaxHistorySize = 10;
	
};
