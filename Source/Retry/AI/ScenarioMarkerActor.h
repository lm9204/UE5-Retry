#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/ScenarioMarkerTypes.h"
#include "ScenarioMarkerActor.generated.h"

class USceneComponent;

UCLASS(Abstract)
class RETRY_API AScenarioMarkerActor : public AActor
{
	GENERATED_BODY()

public:
	AScenarioMarkerActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scenario|Marker")
	FName MarkerId;

	UFUNCTION(BlueprintPure, Category="Scenario|Marker")
	FName GetMarkerId() const;

	static FScenarioMarkerValidationResult ValidateMarkerSet(
		const TArray<AScenarioMarkerActor*>& Markers);

protected:
	virtual void AppendValidationIssues(
		const TSet<FName>& ObjectiveIds,
		FScenarioMarkerValidationResult& Result) const;

	static void AddValidationIssue(
		FScenarioMarkerValidationResult& Result,
		EScenarioMarkerValidationErrorCode Code,
		FName MarkerId,
		const FText& Message);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Marker")
	TObjectPtr<USceneComponent> SceneRoot;
};
