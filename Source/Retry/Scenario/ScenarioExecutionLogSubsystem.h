#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AI/CommandTypes.h"
#include "Scenario/ScenarioTypes.h"
#include "ScenarioExecutionLogSubsystem.generated.h"

UENUM(BlueprintType)
enum class EScenarioExecutionEventType : uint8
{
	RunStarted,
	RunEnded,
	CommandValidated,
	CommandValidationRejected,
	CommandStatusChanged,
};

UENUM(BlueprintType)
enum class EScenarioRunEndReason : uint8
{
	None,
	Restarted,
	ReturnedToMenu,
	Replaced,
	GameInstanceShutdown,
};

USTRUCT(BlueprintType)
struct FScenarioExecutionEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	FGuid EventId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	FGuid CommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	int64 SequenceNumber = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	FDateTime TimestampUtc;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	EScenarioExecutionEventType EventType =
		EScenarioExecutionEventType::RunStarted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	FName GroupId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	bool bHasStatusTransition = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	ECommandStatus PreviousStatus = ECommandStatus::Proposed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	ECommandStatus NewStatus = ECommandStatus::Proposed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	FName ResultCode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	FString Message;
};

USTRUCT(BlueprintType)
struct FScenarioExecutionRunLog
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	FName ScenarioId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	FDateTime StartedAtUtc;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	FDateTime EndedAtUtc;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	bool bIsActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	EScenarioRunEndReason EndReason = EScenarioRunEndReason::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Log")
	TArray<FScenarioExecutionEvent> Events;
};

UCLASS()
class RETRY_API UScenarioExecutionLogSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	bool StartRun(
		const FScenarioRunContext& RunContext,
		EScenarioRunEndReason ReplacementReason =
			EScenarioRunEndReason::Replaced);

	bool EndActiveRun(EScenarioRunEndReason Reason);

	FGuid RecordCommandEvent(
		const FGuid& RunId,
		const FGuid& CommandId,
		FName GroupId,
		EScenarioExecutionEventType EventType,
		FName ResultCode,
		const FString& Message);

	FGuid RecordCommandStatusTransition(
		const FGuid& RunId,
		const FGuid& CommandId,
		FName GroupId,
		ECommandStatus PreviousStatus,
		ECommandStatus NewStatus,
		FName ResultCode,
		const FString& Message);

	UFUNCTION(BlueprintPure, Category="Scenario|Log")
	bool HasActiveRun() const;

	UFUNCTION(BlueprintPure, Category="Scenario|Log")
	bool IsRecordingRun(const FGuid& RunId) const;

	UFUNCTION(BlueprintPure, Category="Scenario|Log")
	FScenarioExecutionRunLog GetActiveRunLog() const;

	UFUNCTION(BlueprintPure, Category="Scenario|Log")
	TArray<FScenarioExecutionRunLog> GetCompletedRunLogs() const;

	void ResetAllLogs();

private:
	FGuid AddEvent(FScenarioExecutionEvent Event);
	void AddRunLifecycleEvent(EScenarioExecutionEventType EventType);

	UPROPERTY(Transient)
	FScenarioExecutionRunLog ActiveRunLog;

	UPROPERTY(Transient)
	TArray<FScenarioExecutionRunLog> CompletedRunLogs;

	int64 NextSequenceNumber = 1;
};
