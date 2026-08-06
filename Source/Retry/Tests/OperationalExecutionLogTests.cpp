#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Scenario/ScenarioExecutionLogSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace OperationalExecutionLogTests
{
	struct FFixture
	{
		FScenarioRunContext Run;
		UScenarioExecutionLogSubsystem* Log = nullptr;

		FFixture()
		{
			Run.RunId = FGuid::NewGuid();
			Run.ScenarioId = TEXT("OperationalLogTest");
			Run.Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(
				TEXT("/Game/Tests/DummyScenarioLevel.DummyScenarioLevel")));
			Run.bIsActive = true;
			UGameInstance* GameInstance = NewObject<UGameInstance>();
			Log = NewObject<UScenarioExecutionLogSubsystem>(GameInstance);
			Log->StartRun(Run);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOperationalExecutionLogLinksIds,
	"Retry.Operational.ExecutionLog.LinksFactReportAndCommandIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOperationalExecutionLogLinksIds::RunTest(
	const FString& Parameters)
{
	OperationalExecutionLogTests::FFixture Fixture;
	const FGuid CommandId = FGuid::NewGuid();
	const FGuid FactId = FGuid::NewGuid();
	const FGuid ReportId = FGuid::NewGuid();
	TestTrue(TEXT("Fact event is recorded."),
		Fixture.Log->RecordOperationalEvent(
			Fixture.Run.RunId,
			CommandId,
			TEXT("A"),
			EScenarioExecutionEventType::OperationalFactObserved,
			FactId,
			ReportId,
			TEXT("Observed"),
			TEXT("Observed.")).IsValid());
	TestTrue(TEXT("Report received event is recorded."),
		Fixture.Log->RecordOperationalEvent(
			Fixture.Run.RunId,
			CommandId,
			TEXT("A"),
			EScenarioExecutionEventType::OperationalReportReceived,
			FGuid(),
			ReportId,
			TEXT("Received"),
			TEXT("Received.")).IsValid());

	const FScenarioExecutionRunLog RunLog = Fixture.Log->GetActiveRunLog();
	TestEqual(TEXT("Run start and two operational events are stored."),
		RunLog.Events.Num(), 3);
	if (RunLog.Events.Num() != 3)
	{
		return false;
	}
	TestEqual(TEXT("Fact identity is preserved."),
		RunLog.Events[1].FactId, FactId);
	TestEqual(TEXT("Report identity links both operational events."),
		RunLog.Events[1].ReportId, RunLog.Events[2].ReportId);
	TestEqual(TEXT("Command identity links the report chain."),
		RunLog.Events[1].CommandId, RunLog.Events[2].CommandId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOperationalExecutionLogRejectsStaleRun,
	"Retry.Operational.ExecutionLog.RejectsStaleRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOperationalExecutionLogRejectsStaleRun::RunTest(
	const FString& Parameters)
{
	OperationalExecutionLogTests::FFixture Fixture;
	TestFalse(TEXT("Another Run cannot write an operational event."),
		Fixture.Log->RecordOperationalEvent(
			FGuid::NewGuid(),
			FGuid::NewGuid(),
			TEXT("A"),
			EScenarioExecutionEventType::OperationalReportCreated,
			FGuid(),
			FGuid::NewGuid(),
			TEXT("Stale"),
			TEXT("Stale.")).IsValid());
	TestEqual(TEXT("Only RunStarted remains."),
		Fixture.Log->GetActiveRunLog().Events.Num(), 1);
	return true;
}

#endif
