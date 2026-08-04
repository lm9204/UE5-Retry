#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Scenario/ScenarioTypes.h"
#include "ScenarioDebugWidget.generated.h"

/**
 * C++ boundary for the in-game scenario debug panel.
 * The Widget Blueprint owns layout while this class owns safe runtime actions.
 */
UCLASS(Abstract, Blueprintable)
class RETRY_API UScenarioDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Scenario|Debug")
	void RefreshRunContext();

	UFUNCTION(BlueprintCallable, Category="Scenario|Debug")
	bool RestartCurrentScenario();

	UFUNCTION(BlueprintCallable, Category="Scenario|Debug")
	bool ReturnToScenarioMenu();

	UFUNCTION(BlueprintPure, Category="Scenario|Debug")
	FScenarioRunContext GetDisplayedRunContext() const;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category="Scenario|Debug", meta=(DisplayName="Run Context Changed"))
	void BP_OnRunContextChanged(const FScenarioRunContext& RunContext);

	UFUNCTION(BlueprintImplementableEvent, Category="Scenario|Debug", meta=(DisplayName="Scenario Action Failed"))
	void BP_OnScenarioActionFailed(const FText& Message);

private:
	UPROPERTY(Transient)
	FScenarioRunContext DisplayedRunContext;
};
