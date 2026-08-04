#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Scenario/ScenarioTypes.h"
#include "ScenarioSelectWidget.generated.h"

class UScenarioDefinition;

/**
 * C++ boundary for the Blueprint-authored scenario selection screen.
 * Visual layout and list entry creation remain in Widget Blueprint.
 */
UCLASS(Abstract, Blueprintable)
class RETRY_API UScenarioSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Scenario")
	void RefreshScenarioList();

	UFUNCTION(BlueprintCallable, Category="Scenario")
	bool SelectScenario(UScenarioDefinition* Scenario);

	UFUNCTION(BlueprintCallable, Category="Scenario")
	void SetSelectedLaunchOptions(const FScenarioLaunchOptions& LaunchOptions);

	UFUNCTION(BlueprintCallable, Category="Scenario")
	bool StartSelectedScenario();

	UFUNCTION(BlueprintPure, Category="Scenario")
	TArray<UScenarioDefinition*> GetAvailableScenarios() const;

	UFUNCTION(BlueprintPure, Category="Scenario")
	UScenarioDefinition* GetSelectedScenario() const;

	UFUNCTION(BlueprintPure, Category="Scenario")
	FScenarioLaunchOptions GetSelectedLaunchOptions() const;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category="Scenario", meta=(DisplayName="Refresh Scenario List"))
	void BP_RefreshScenarioList(const TArray<UScenarioDefinition*>& Scenarios);

	UFUNCTION(BlueprintImplementableEvent, Category="Scenario", meta=(DisplayName="Scenario Selection Changed"))
	void BP_OnScenarioSelectionChanged(
		UScenarioDefinition* Scenario,
		const FScenarioLaunchOptions& LaunchOptions);

	UFUNCTION(BlueprintImplementableEvent, Category="Scenario", meta=(DisplayName="Scenario Start Failed"))
	void BP_OnScenarioStartFailed(const FText& Message);

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UScenarioDefinition>> AvailableScenarios;

	UPROPERTY(Transient)
	TObjectPtr<UScenarioDefinition> SelectedScenario;

	UPROPERTY(Transient)
	FScenarioLaunchOptions SelectedLaunchOptions;
};
