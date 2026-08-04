#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Scenario/ScenarioTypes.h"
#include "ScenarioInitializer.generated.h"

class UScenarioDefinition;

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

private:
	EScenarioInitializationResult ValidateSetup(
		bool bRequireActiveRunContext, FText& OutMessage) const;
	bool ValidatePlacedActors(FText& OutMessage) const;
	void ResetRuntimeState() const;
};
