#pragma once

#include "CoreMinimal.h"
#include "AI/ScenarioMarkerActor.h"
#include "ObjectiveAreaActor.generated.h"

class USphereComponent;

UCLASS()
class RETRY_API AObjectiveAreaActor : public AScenarioMarkerActor
{
	GENERATED_BODY()

public:
	AObjectiveAreaActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scenario|Objective",
		meta=(ClampMin="1.0", UIMin="1.0"))
	float AreaRadius = 500.f;

	UFUNCTION(BlueprintPure, Category="Scenario|Objective")
	float GetAreaRadius() const;

protected:
	virtual void AppendValidationIssues(
		const TSet<FName>& ObjectiveIds,
		FScenarioMarkerValidationResult& Result) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Objective")
	TObjectPtr<USphereComponent> AreaVisualization;
};
