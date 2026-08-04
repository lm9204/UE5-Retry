#pragma once

#include "CoreMinimal.h"
#include "ScenarioTypes.generated.h"

class UScenarioDefinition;
class UWorld;

UENUM(BlueprintType)
enum class EScenarioInitializationResult : uint8
{
	NotStarted,
	Succeeded,
	MissingDefinition,
	InvalidDefinition,
	MissingRunContext,
	ScenarioMismatch,
	LevelMismatch,
	InvalidActorConfiguration,
};

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

/**
 * 메뉴에서 선택한 값이 Level 전환을 넘어 한 번의 Scenario 실행에 전달되는 상태다.
 */
USTRUCT(BlueprintType)
struct FScenarioRunContext
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario")
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario")
	FName ScenarioId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario")
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario")
	FScenarioLaunchOptions LaunchOptions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario")
	bool bIsActive = false;

	bool IsValid() const
	{
		return bIsActive && RunId.IsValid() && !ScenarioId.IsNone() && !Level.IsNull();
	}
};

RETRY_API bool TryCreateScenarioRunContext(
	const UScenarioDefinition* Definition,
	const FScenarioLaunchOptions& LaunchOptions,
	FScenarioRunContext& OutContext,
	FText& OutError);
