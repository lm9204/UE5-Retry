#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Scenario/ScenarioTypes.h"
#include "ScenarioDefinition.generated.h"

class UWorld;

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

	UFUNCTION(BlueprintPure, Category="Scenario|Validation")
	bool IsDefinitionValid(FText& OutError) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
