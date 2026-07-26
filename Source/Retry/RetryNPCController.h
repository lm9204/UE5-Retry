#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "RetryNPCCharacter.h"
#include "Perception/AIPerceptionTypes.h"
#include "RetryNPCController.generated.h"

UCLASS()
class RETRY_API ARetryNPCController : public AAIController
{
	GENERATED_BODY()

public:
	ARetryNPCController(const FObjectInitializer& ObjectInitializer);

	virtual FGenericTeamId GetGenericTeamId() const override
	{
		if (ARetryNPCCharacter* NPC = Cast<ARetryNPCCharacter>(GetPawn()))
			return FGenericTeamId(NPC->TeamID);
		return FGenericTeamId(2);
	}

	UPROPERTY(EditDefaultsOnly, Category="AI")
	class UBehaviorTree* BehaviorTreeAsset;

	UPROPERTY(VisibleAnywhere, Category="AI")
	class UNPCDecisionComponent* DecisionComponent;

	UPROPERTY()
	AActor* LastPerceivedActor = nullptr;

	float LastPerceptionTime = 0.f;
	float LastLostTime = 0.f;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

private:
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};