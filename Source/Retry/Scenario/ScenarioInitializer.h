#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Scenario/ScenarioDefinition.h"
#include "Scenario/ScenarioTypes.h"
#include "ScenarioInitializer.generated.h"

class AGroupManagerActor;
class UScenarioExecutionLogSubsystem;

/**
 * Validates a scenario level and applies per-run state when the level is entered
 * through UScenarioRuntimeSubsystem.
 */
UCLASS()
class RETRY_API AScenarioInitializer : public AActor
{
	GENERATED_BODY()

public:
	AScenarioInitializer();

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Scenario")
	TSoftObjectPtr<UScenarioDefinition> ScenarioDefinition;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Scenario|Validation")
	EScenarioInitializationResult LastInitializationResult =
		EScenarioInitializationResult::NotStarted;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Scenario|Validation")
	FText LastValidationMessage;

	UFUNCTION(CallInEditor, Category="Scenario|Validation")
	void ValidateScenarioSetup();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	EScenarioInitializationResult ValidateSetup(
		bool bRequireActiveRunContext, FText& OutMessage) const;
	bool ValidatePlacedActors(FText& OutMessage) const;
	void ResetRuntimeState() const;
	void StartOpeningOrders();
	bool StartScenarioCommand(
		const FCommandIntent& Command,
		AGroupManagerActor* Group,
		const FGuid& RunId,
		UScenarioExecutionLogSubsystem* ExecutionLog,
		const TCHAR* OrderLabel);
	void BeginFollowUpOrderMonitoring(
		TArray<FScenarioFollowUpOrder> Orders,
		const FGuid& RunId);
	void EvaluateFollowUpOrders();
	void StopFollowUpOrderMonitoring();
	void BeginCommanderObjectiveMonitoring(
		TArray<FScenarioOperationalObjective> Objectives,
		const FGuid& RunId);
	void EvaluateCommanderObjectives();
	void StopCommanderObjectiveMonitoring();

	UPROPERTY(Transient)
	TArray<FScenarioFollowUpOrder> PendingFollowUpOrders;

	FGuid FollowUpRunId;
	FTimerHandle FollowUpEvaluationTimer;

	UPROPERTY(Transient)
	TArray<FScenarioOperationalObjective> PendingOperationalObjectives;

	FGuid CommanderRunId;
	FTimerHandle CommanderEvaluationTimer;
};
