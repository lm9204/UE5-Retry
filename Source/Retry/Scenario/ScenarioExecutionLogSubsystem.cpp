#include "Scenario/ScenarioExecutionLogSubsystem.h"

void UScenarioExecutionLogSubsystem::Deinitialize()
{
	EndActiveRun(EScenarioRunEndReason::GameInstanceShutdown);
	Super::Deinitialize();
}

bool UScenarioExecutionLogSubsystem::StartRun(
	const FScenarioRunContext& RunContext,
	const EScenarioRunEndReason ReplacementReason)
{
	if (!RunContext.IsValid())
	{
		return false;
	}

	if (ActiveRunLog.bIsActive)
	{
		EndActiveRun(ReplacementReason);
	}

	ActiveRunLog = FScenarioExecutionRunLog();
	ActiveRunLog.RunId = RunContext.RunId;
	ActiveRunLog.ScenarioId = RunContext.ScenarioId;
	ActiveRunLog.StartedAtUtc = FDateTime::UtcNow();
	ActiveRunLog.bIsActive = true;
	NextSequenceNumber = 1;
	AddRunLifecycleEvent(EScenarioExecutionEventType::RunStarted);
	return true;
}

bool UScenarioExecutionLogSubsystem::EndActiveRun(
	const EScenarioRunEndReason Reason)
{
	if (!ActiveRunLog.bIsActive || Reason == EScenarioRunEndReason::None)
	{
		return false;
	}

	AddRunLifecycleEvent(EScenarioExecutionEventType::RunEnded);
	ActiveRunLog.EndedAtUtc = FDateTime::UtcNow();
	ActiveRunLog.EndReason = Reason;
	ActiveRunLog.bIsActive = false;
	CompletedRunLogs.Add(ActiveRunLog);
	ActiveRunLog = FScenarioExecutionRunLog();
	NextSequenceNumber = 1;
	return true;
}

FGuid UScenarioExecutionLogSubsystem::RecordCommandEvent(
	const FGuid& RunId,
	const FGuid& CommandId,
	const FName GroupId,
	const EScenarioExecutionEventType EventType,
	const FName ResultCode,
	const FString& Message)
{
	const bool bSupportedCommandEvent =
		EventType == EScenarioExecutionEventType::CommandValidated
		|| EventType == EScenarioExecutionEventType::CommandValidationRejected;
	if (!ActiveRunLog.bIsActive
		|| ActiveRunLog.RunId != RunId
		|| !CommandId.IsValid()
		|| !bSupportedCommandEvent)
	{
		return FGuid();
	}

	FScenarioExecutionEvent Event;
	Event.CommandId = CommandId;
	Event.GroupId = GroupId;
	Event.EventType = EventType;
	Event.ResultCode = ResultCode;
	Event.Message = Message;
	return AddEvent(MoveTemp(Event));
}

FGuid UScenarioExecutionLogSubsystem::RecordCommandStatusTransition(
	const FGuid& RunId,
	const FGuid& CommandId,
	const FName GroupId,
	const ECommandStatus PreviousStatus,
	const ECommandStatus NewStatus,
	const FName ResultCode,
	const FString& Message)
{
	if (!CanTransitionCommandStatus(PreviousStatus, NewStatus))
	{
		return FGuid();
	}

	FScenarioExecutionEvent Event;
	Event.CommandId = CommandId;
	Event.GroupId = GroupId;
	Event.EventType = EScenarioExecutionEventType::CommandStatusChanged;
	Event.bHasStatusTransition = true;
	Event.PreviousStatus = PreviousStatus;
	Event.NewStatus = NewStatus;
	Event.ResultCode = ResultCode;
	Event.Message = Message;

	if (!ActiveRunLog.bIsActive
		|| ActiveRunLog.RunId != RunId
		|| !CommandId.IsValid())
	{
		return FGuid();
	}

	return AddEvent(MoveTemp(Event));
}

FGuid UScenarioExecutionLogSubsystem::RecordOperationalEvent(
	const FGuid& RunId,
	const FGuid& CommandId,
	const FName GroupId,
	const EScenarioExecutionEventType EventType,
	const FGuid& FactId,
	const FGuid& ReportId,
	const FName ResultCode,
	const FString& Message)
{
	const bool bFactEvent =
		EventType == EScenarioExecutionEventType::OperationalFactObserved;
	const bool bReportEvent =
		EventType == EScenarioExecutionEventType::OperationalReportCreated
		|| EventType == EScenarioExecutionEventType::OperationalReportReceived;
	if (!IsRecordingRun(RunId)
		|| !CommandId.IsValid()
		|| (!bFactEvent && !bReportEvent)
		|| (bFactEvent && (!FactId.IsValid() || !ReportId.IsValid()))
		|| (bReportEvent && !ReportId.IsValid()))
	{
		return FGuid();
	}

	FScenarioExecutionEvent Event;
	Event.CommandId = CommandId;
	Event.FactId = FactId;
	Event.ReportId = ReportId;
	Event.GroupId = GroupId;
	Event.EventType = EventType;
	Event.ResultCode = ResultCode;
	Event.Message = Message;
	return AddEvent(MoveTemp(Event));
}

bool UScenarioExecutionLogSubsystem::HasActiveRun() const
{
	return ActiveRunLog.bIsActive;
}

bool UScenarioExecutionLogSubsystem::IsRecordingRun(const FGuid& RunId) const
{
	return ActiveRunLog.bIsActive
		&& RunId.IsValid()
		&& ActiveRunLog.RunId == RunId;
}

FScenarioExecutionRunLog
UScenarioExecutionLogSubsystem::GetActiveRunLog() const
{
	return ActiveRunLog;
}

TArray<FScenarioExecutionRunLog>
UScenarioExecutionLogSubsystem::GetCompletedRunLogs() const
{
	return CompletedRunLogs;
}

void UScenarioExecutionLogSubsystem::ResetAllLogs()
{
	ActiveRunLog = FScenarioExecutionRunLog();
	CompletedRunLogs.Reset();
	NextSequenceNumber = 1;
}

FGuid UScenarioExecutionLogSubsystem::AddEvent(
	FScenarioExecutionEvent Event)
{
	Event.EventId = FGuid::NewGuid();
	Event.RunId = ActiveRunLog.RunId;
	Event.SequenceNumber = NextSequenceNumber++;
	Event.TimestampUtc = FDateTime::UtcNow();
	ActiveRunLog.Events.Add(MoveTemp(Event));
	return ActiveRunLog.Events.Last().EventId;
}

void UScenarioExecutionLogSubsystem::AddRunLifecycleEvent(
	const EScenarioExecutionEventType EventType)
{
	FScenarioExecutionEvent Event;
	Event.EventType = EventType;
	AddEvent(MoveTemp(Event));
}
