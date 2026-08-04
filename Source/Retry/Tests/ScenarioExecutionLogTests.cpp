#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Scenario/ScenarioExecutionLogSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ScenarioExecutionLogTests
{
	FScenarioRunContext MakeRunContext(const FName ScenarioId)
	{
		FScenarioRunContext Context;
		Context.RunId = FGuid::NewGuid();
		Context.ScenarioId = ScenarioId;
		Context.Level = TSoftObjectPtr<UWorld>(
			FSoftObjectPath(TEXT("/Game/Tests/DummyScenarioLevel.DummyScenarioLevel")));
		Context.bIsActive = true;
		return Context;
	}

	UScenarioExecutionLogSubsystem* MakeExecutionLog()
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		return NewObject<UScenarioExecutionLogSubsystem>(GameInstance);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioExecutionLogLinksRunCommandAndEventIds,
	"Retry.Scenario.ExecutionLog.LinksRunCommandAndEventIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioExecutionLogLinksRunCommandAndEventIds::RunTest(
	const FString& Parameters)
{
	UScenarioExecutionLogSubsystem* Log =
		ScenarioExecutionLogTests::MakeExecutionLog();
	const FScenarioRunContext Context =
		ScenarioExecutionLogTests::MakeRunContext(TEXT("TraceTest"));
	const FGuid CommandId = FGuid::NewGuid();

	TestTrue(TEXT("유효한 Run Log를 시작한다."), Log->StartRun(Context));
	const FGuid BypassedTransitionId = Log->RecordCommandEvent(
		Context.RunId,
		CommandId,
		TEXT("GroupA"),
		EScenarioExecutionEventType::CommandStatusChanged,
		TEXT("InvalidApi"),
		TEXT("Status transitions require the dedicated API."));
	TestFalse(TEXT("일반 event API의 상태 전이 기록을 거부한다."),
		BypassedTransitionId.IsValid());

	const FGuid EventId = Log->RecordCommandStatusTransition(
		Context.RunId,
		CommandId,
		TEXT("GroupA"),
		ECommandStatus::Proposed,
		ECommandStatus::Validated,
		TEXT("ValidationSucceeded"),
		TEXT("Command validated."));
	TestTrue(TEXT("Command event ID를 생성한다."), EventId.IsValid());

	const FScenarioExecutionRunLog RunLog = Log->GetActiveRunLog();
	TestEqual(TEXT("RunStarted와 Command event 두 건을 기록한다."),
		RunLog.Events.Num(), 2);
	const FScenarioExecutionEvent& Event = RunLog.Events.Last();
	TestEqual(TEXT("Event가 Run ID를 보존한다."), Event.RunId, Context.RunId);
	TestEqual(TEXT("Event가 Command ID를 보존한다."), Event.CommandId, CommandId);
	TestEqual(TEXT("Event ID가 반환값과 일치한다."), Event.EventId, EventId);
	TestEqual(TEXT("Run 안에서 순번이 증가한다."), Event.SequenceNumber, int64(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioExecutionLogRejectsStaleRunWrites,
	"Retry.Scenario.ExecutionLog.RejectsStaleRunWrites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioExecutionLogRejectsStaleRunWrites::RunTest(
	const FString& Parameters)
{
	UScenarioExecutionLogSubsystem* Log =
		ScenarioExecutionLogTests::MakeExecutionLog();
	const FScenarioRunContext Context =
		ScenarioExecutionLogTests::MakeRunContext(TEXT("CurrentRun"));
	Log->StartRun(Context);

	const FGuid EventId = Log->RecordCommandEvent(
		FGuid::NewGuid(),
		FGuid::NewGuid(),
		TEXT("GroupA"),
		EScenarioExecutionEventType::CommandValidationRejected,
		TEXT("StaleRun"),
		TEXT("This event must be rejected."));
	TestFalse(TEXT("현재 Run과 다른 event write를 거부한다."), EventId.IsValid());
	TestEqual(TEXT("거부된 event는 buffer에 추가하지 않는다."),
		Log->GetActiveRunLog().Events.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioExecutionLogPreservesCompletedRuns,
	"Retry.Scenario.ExecutionLog.PreservesCompletedRuns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioExecutionLogPreservesCompletedRuns::RunTest(
	const FString& Parameters)
{
	UScenarioExecutionLogSubsystem* Log =
		ScenarioExecutionLogTests::MakeExecutionLog();
	const FScenarioRunContext FirstRun =
		ScenarioExecutionLogTests::MakeRunContext(TEXT("FirstRun"));
	const FScenarioRunContext SecondRun =
		ScenarioExecutionLogTests::MakeRunContext(TEXT("SecondRun"));

	Log->StartRun(FirstRun);
	Log->StartRun(SecondRun, EScenarioRunEndReason::Restarted);

	const TArray<FScenarioExecutionRunLog> CompletedRuns =
		Log->GetCompletedRunLogs();
	TestEqual(TEXT("교체된 첫 Run을 보존한다."), CompletedRuns.Num(), 1);
	TestEqual(TEXT("첫 Run의 종료 이유를 기록한다."),
		CompletedRuns[0].EndReason, EScenarioRunEndReason::Restarted);
	TestEqual(TEXT("첫 Run에 Start/End event를 기록한다."),
		CompletedRuns[0].Events.Num(), 2);
	TestEqual(TEXT("두 번째 Run이 active다."),
		Log->GetActiveRunLog().RunId, SecondRun.RunId);
	return true;
}

#endif
