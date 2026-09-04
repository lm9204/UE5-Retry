#pragma once

#include "CoreMinimal.h"
#include "AI/CommandTypes.h"
#include "AI/OperationalObjectiveTypes.h"
#include "Engine/DataAsset.h"
#include "Scenario/ScenarioTypes.h"
#include "ScenarioDefinition.generated.h"

class UWorld;

/** A team-local operational Fact that must be received before a scripted order may start. */
USTRUCT(BlueprintType)
struct FScenarioFactCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario|Follow Up")
	FName PredicateId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario|Follow Up")
	FName SubjectId;

	/** None accepts the Fact from any group on the receiving command's team. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario|Follow Up")
	FName SourceGroupId;
};

/** A deterministic designer-authored command gated by received operational Facts. */
USTRUCT(BlueprintType)
struct FScenarioFollowUpOrder
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario|Follow Up")
	FCommandIntent Command;

	/** All conditions must be present in the assigned group's Team Memory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario|Follow Up")
	TArray<FScenarioFactCondition> RequiredFacts;
};

/** A designer-authored desired state activated by received operational Facts. */
USTRUCT(BlueprintType)
struct FScenarioOperationalObjective
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario|Operational Objective")
	FName ObjectiveId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario|Operational Objective")
	FName DesiredStateId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario|Operational Objective")
	FName SubjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario|Operational Objective")
	uint8 TeamId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario|Operational Objective",
		meta=(ClampMin="0", ClampMax="100"))
	int32 Priority = 50;

	/** All conditions must be present before this objective may be planned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario|Operational Objective")
	TArray<FScenarioFactCondition> ActivationFacts;
};

/**
 * 메뉴에 표시하고 실행할 Scenario 한 개의 정적 설정이다.
 * NPC와 Group 배치는 초기 버전에서 Level이 소유하며, 이 asset은 실행 정보를 가리킨다.
 */
UCLASS(BlueprintType)
class RETRY_API UScenarioDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenario")
	FName ScenarioId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenario")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenario", meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenario")
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenario")
	FScenarioLaunchOptions DefaultLaunchOptions;

	/** Initial in-world orders issued by the scenario's HQ at run start. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenario|Opening Orders")
	TArray<FCommandIntent> OpeningOrders;

	/** Scripted commands evaluated in authored order after their Facts are received. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenario|Follow Up Orders")
	TArray<FScenarioFollowUpOrder> FollowUpOrders;

	/** Desired HQ states that the Commander turns into runtime Commands. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scenario|Operational Objectives")
	TArray<FScenarioOperationalObjective> OperationalObjectives;

	UFUNCTION(BlueprintPure, Category="Scenario|Validation")
	bool IsDefinitionValid(FText& OutError) const;

	bool BuildOpeningOrders(
		TArray<FCommandIntent>& OutCommands,
		FText& OutError) const;

	bool BuildFollowUpOrders(
		TArray<FScenarioFollowUpOrder>& OutOrders,
		FText& OutError) const;

	bool BuildOperationalObjectives(
		TArray<FScenarioOperationalObjective>& OutObjectives,
		FText& OutError) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
