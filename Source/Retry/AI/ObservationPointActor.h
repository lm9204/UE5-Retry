#pragma once

#include "CoreMinimal.h"
#include "AI/ScenarioMarkerActor.h"
#include "ObservationPointActor.generated.h"

class UArrowComponent;

UCLASS()
class RETRY_API AObservationPointActor : public AScenarioMarkerActor
{
	GENERATED_BODY()

public:
	AObservationPointActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scenario|Observation")
	FName ObjectiveId;

	UFUNCTION(BlueprintPure, Category="Scenario|Observation")
	FName GetObjectiveId() const;

protected:
	virtual void AppendValidationIssues(
		const TSet<FName>& ObjectiveIds,
		FScenarioMarkerValidationResult& Result) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Observation")
	TObjectPtr<UArrowComponent> FacingVisualization;
};
