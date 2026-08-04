#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Scenario/ScenarioTypes.h"
#include "ScenarioRuntimeSubsystem.generated.h"

class UScenarioDefinition;
enum class EScenarioRunEndReason : uint8;

/**
 * 메뉴 선택과 현재 Scenario 실행 상태를 Level 전환 너머로 전달한다.
 */
UCLASS()
class RETRY_API UScenarioRuntimeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category="Scenario")
	TArray<UScenarioDefinition*> GetRegisteredScenarios() const;

	UFUNCTION(BlueprintCallable, Category="Scenario")
	bool StartScenario(FName ScenarioId, const FScenarioLaunchOptions& LaunchOptions);

	UFUNCTION(BlueprintCallable, Category="Scenario")
	bool RestartCurrentScenario();

	UFUNCTION(BlueprintCallable, Category="Scenario")
	bool ReturnToScenarioMenu();

	UFUNCTION(BlueprintPure, Category="Scenario")
	FScenarioRunContext GetCurrentRunContext() const;

	UFUNCTION(BlueprintPure, Category="Scenario")
	bool IsScenarioActive() const;

private:
	bool StartScenarioInternal(
		FName ScenarioId,
		const FScenarioLaunchOptions& LaunchOptions,
		EScenarioRunEndReason PreviousRunEndReason);
	bool LoadValidatedScenarios(TArray<UScenarioDefinition*>& OutScenarios) const;
	UScenarioDefinition* FindRegisteredScenario(FName ScenarioId) const;
	void ResetLLMQueueForTransition() const;

	UPROPERTY(Transient)
	FScenarioRunContext CurrentRunContext;
};
