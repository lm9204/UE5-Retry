#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ScenarioRegistrySettings.generated.h"

class UScenarioDefinition;

/**
 * 메뉴와 RuntimeSubsystem이 사용할 Scenario의 명시적 등록 목록이다.
 * 자동 asset 검색 대신 Project Settings에 등록된 항목만 실행 대상으로 인정한다.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Scenario Registry"))
class RETRY_API UScenarioRegistrySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category="Scenario")
	TArray<TSoftObjectPtr<UScenarioDefinition>> RegisteredScenarios;

	bool IsRegistryValid(TArray<FText>& OutErrors) const;

	virtual FName GetCategoryName() const override;
};
