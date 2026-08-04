#include "Misc/AutomationTest.h"

#include "Engine/World.h"

#include "Scenario/ScenarioDefinition.h"
#include "Scenario/ScenarioRegistrySettings.h"
#include "Scenario/ScenarioRuntimeSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ScenarioRuntimeTests
{
	UScenarioDefinition* MakeValidDefinition(const FName ScenarioId)
	{
		UScenarioDefinition* Definition = NewObject<UScenarioDefinition>();
		Definition->ScenarioId = ScenarioId;
		Definition->DisplayName = FText::FromName(ScenarioId);
		Definition->Level = TSoftObjectPtr<UWorld>(
			FSoftObjectPath(TEXT("/Game/Tests/DummyScenarioLevel.DummyScenarioLevel")));
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioRegistryRejectsInvalidEntries,
	"Retry.Scenario.Registry.RejectsInvalidEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioRegistryRejectsInvalidEntries::RunTest(const FString& Parameters)
{
	UScenarioRegistrySettings* Registry = NewObject<UScenarioRegistrySettings>();
	Registry->RegisteredScenarios.Reset();
	Registry->RegisteredScenarios.Add(TSoftObjectPtr<UScenarioDefinition>());

	UScenarioDefinition* MissingLevel = NewObject<UScenarioDefinition>();
	MissingLevel->ScenarioId = TEXT("MissingLevel");
	MissingLevel->DisplayName = FText::FromString(TEXT("Missing Level"));
	Registry->RegisteredScenarios.Add(TSoftObjectPtr<UScenarioDefinition>(MissingLevel));

	TArray<FText> Errors;
	TestFalse(TEXT("빈 참조와 Level 누락을 포함한 Registry는 유효하지 않다."),
		Registry->IsRegistryValid(Errors));
	TestEqual(TEXT("두 잘못된 항목을 모두 보고한다."), Errors.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioRegistryRejectsDuplicateIds,
	"Retry.Scenario.Registry.RejectsDuplicateIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioRegistryRejectsDuplicateIds::RunTest(const FString& Parameters)
{
	UScenarioRegistrySettings* Registry = NewObject<UScenarioRegistrySettings>();
	Registry->RegisteredScenarios.Reset();
	Registry->RegisteredScenarios.Add(TSoftObjectPtr<UScenarioDefinition>(
		ScenarioRuntimeTests::MakeValidDefinition(TEXT("Duplicate"))));
	Registry->RegisteredScenarios.Add(TSoftObjectPtr<UScenarioDefinition>(
		ScenarioRuntimeTests::MakeValidDefinition(TEXT("Duplicate"))));

	TArray<FText> Errors;
	TestFalse(TEXT("같은 Scenario ID를 두 번 등록할 수 없다."),
		Registry->IsRegistryValid(Errors));
	TestEqual(TEXT("중복 ID 오류 한 건을 보고한다."), Errors.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioRunContextPreservesLaunchData,
	"Retry.Scenario.Runtime.PreservesLaunchData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioRunContextPreservesLaunchData::RunTest(const FString& Parameters)
{
	UScenarioDefinition* Definition =
		ScenarioRuntimeTests::MakeValidDefinition(TEXT("RuntimeContext"));

	FScenarioLaunchOptions LaunchOptions;
	LaunchOptions.Seed = 4242;
	LaunchOptions.bUseLLM = true;
	LaunchOptions.bEnableLogging = false;
	LaunchOptions.bAutoStart = false;

	FScenarioRunContext Context;
	FText Error;
	TestTrue(TEXT("유효한 Definition으로 Run Context를 만든다."),
		TryCreateScenarioRunContext(Definition, LaunchOptions, Context, Error));
	TestTrue(TEXT("생성된 Run Context가 유효하다."), Context.IsValid());
	TestEqual(TEXT("Scenario ID를 보존한다."), Context.ScenarioId, Definition->ScenarioId);
	TestEqual(TEXT("Seed를 보존한다."), Context.LaunchOptions.Seed, 4242);
	TestTrue(TEXT("LLM 옵션을 보존한다."), Context.LaunchOptions.bUseLLM);
	TestFalse(TEXT("Logging 옵션을 보존한다."), Context.LaunchOptions.bEnableLogging);
	TestFalse(TEXT("Auto Start 옵션을 보존한다."), Context.LaunchOptions.bAutoStart);
	TestTrue(TEXT("Level Soft Reference를 보존한다."), Context.Level == Definition->Level);

	const FGuid FirstRunId = Context.RunId;
	TestTrue(TEXT("다시 시작하면 새 Run Context를 만든다."),
		TryCreateScenarioRunContext(Definition, LaunchOptions, Context, Error));
	TestTrue(TEXT("Restart용 Context는 새로운 Run ID를 갖는다."),
		Context.RunId != FirstRunId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioRunContextRejectsInvalidDefinition,
	"Retry.Scenario.Runtime.RejectsInvalidDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioRunContextRejectsInvalidDefinition::RunTest(const FString& Parameters)
{
	UScenarioDefinition* Definition = NewObject<UScenarioDefinition>();
	Definition->ScenarioId = TEXT("MissingLevel");
	Definition->DisplayName = FText::FromString(TEXT("Missing Level"));

	FScenarioRunContext Context;
	FText Error;
	TestFalse(TEXT("Level이 없는 Definition으로 Run Context를 만들 수 없다."),
		TryCreateScenarioRunContext(
			Definition, FScenarioLaunchOptions(), Context, Error));
	TestFalse(TEXT("실패한 Run Context는 활성 상태가 아니다."), Context.bIsActive);
	TestFalse(TEXT("실패 이유를 반환한다."), Error.IsEmpty());
	return true;
}

#endif
