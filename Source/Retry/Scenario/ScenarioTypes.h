#pragma once

#include "CoreMinimal.h"
#include "ScenarioTypes.generated.h"

/**
 * 한 번의 Scenario 실행에 적용할 선택값이다.
 * 메뉴의 사용자 입력과 ScenarioDefinition의 기본값이 이 형태로 합쳐진다.
 */
USTRUCT(BlueprintType)
struct FScenarioLaunchOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario")
	int32 Seed = 1001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario")
	bool bUseLLM = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario")
	bool bEnableLogging = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario")
	bool bAutoStart = true;
};
