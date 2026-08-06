#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CommandValidation.h"
#include "NPCOrderTypes.h"
#include "GroupMemoryTypes.h"
#include "GroupManagerActor.generated.h"

class UScenarioExecutionLogSubsystem;
class UNPCDecisionComponent;

enum class EGroupMissionDispatchOutcome : uint8
{
	Dispatched,
	NoAssignedCommand,
	InvalidCommandState,
	LeaderUnavailable,
	WorldResolutionFailed,
	RecipientUnavailable,
	MissionRejected,
	StatusTransitionFailed,
};

struct FGroupMissionDispatchResult
{
	EGroupMissionDispatchOutcome Outcome =
		EGroupMissionDispatchOutcome::NoAssignedCommand;
	int32 RecipientCount = 0;
	FText Message;

	bool IsSuccess() const
	{
		return Outcome == EGroupMissionDispatchOutcome::Dispatched;
	}
};

UENUM(BlueprintType)
enum class ECommandAssignmentOutcome : uint8
{
	Assigned,
	NoActiveScenario,
	ExecutionLogUnavailable,
	InvalidGroupConfiguration,
	GroupMismatch,
	ActiveCommandExists,
	ValidationRejected,
	TransitionRejected,
};

USTRUCT(BlueprintType)
struct FCommandAssignmentResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Command")
	ECommandAssignmentOutcome Outcome =
		ECommandAssignmentOutcome::NoActiveScenario;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Command")
	FCommandValidationResult Validation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Command")
	FText Message;

	bool IsSuccess() const
	{
		return Outcome == ECommandAssignmentOutcome::Assigned;
	}
};

UCLASS()
class RETRY_API AGroupManagerActor : public AActor
{
	GENERATED_BODY()

public:
	AGroupManagerActor();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category="Group")
	FString GroupID;

	UPROPERTY(EditAnywhere, Category="Group")
	uint8 TeamID = 0;

	UPROPERTY()
	class ARetryNPCCharacter* Leader;

	UPROPERTY()
	TArray<class ARetryNPCCharacter*> Members;

	UFUNCTION(BlueprintCallable, Category="Group")
	void RegisterMember(ARetryNPCCharacter* NPC, bool bIsLeader);

	UFUNCTION(BlueprintCallable, Category="Group")
	void AddGroupMemory(const FString& WitnessID, const FString& EventType,
		FVector Location, float EmotionWeight, const FString& Description);

	UFUNCTION(BlueprintPure, Category="Group")
	TArray<FGroupMemoryEvent> GetRecentGroupMemories(int32 Count) const;

	UFUNCTION(BlueprintCallable, Category="Group")
	void OnLeaderDied();   // 리더의 OnDeath에서 호출

	UFUNCTION(BlueprintCallable, Category="Group")
	void SetOrderForAll(ENPCOrder Order, float Weight);

	UFUNCTION(BlueprintCallable, Category="Group|Command")
	FCommandAssignmentResult AssignCommand(const FCommandIntent& Command);

	UFUNCTION(BlueprintCallable, Category="Group|Command")
	bool TransitionCurrentCommandStatus(
		ECommandStatus NewStatus,
		FName ResultCode,
		const FString& Message);

	UFUNCTION(BlueprintCallable, Category="Group|Command")
	bool CancelCurrentCommand(FName ReasonCode);

	UFUNCTION(BlueprintCallable, Category="Group|Command")
	bool ClearCurrentCommand();

	UFUNCTION(BlueprintPure, Category="Group|Command")
	bool HasCurrentCommand() const;

	UFUNCTION(BlueprintPure, Category="Group|Command")
	FCommandIntent GetCurrentCommand() const;

	FCommandAssignmentResult AssignCommandForRun(
		const FCommandIntent& Command,
		const FGuid& RunId,
		UScenarioExecutionLogSubsystem* ExecutionLog);

	bool TransitionCurrentCommandStatusForRun(
		ECommandStatus NewStatus,
		FName ResultCode,
		const FString& Message,
		const FGuid& RunId,
		UScenarioExecutionLogSubsystem* ExecutionLog);

	UFUNCTION(BlueprintCallable, Category="Group|Mission")
	bool DispatchCurrentReconMission();

	FGroupMissionDispatchResult DispatchCurrentReconMissionForRun(
		const FGuid& RunId,
		UScenarioExecutionLogSubsystem* ExecutionLog);

	FGroupMissionDispatchResult DispatchResolvedMissionForRun(
		const FMissionContext& Mission,
		const TArray<UNPCDecisionComponent*>& Recipients,
		const FGuid& RunId,
		UScenarioExecutionLogSubsystem* ExecutionLog);

	UFUNCTION(BlueprintCallable, Category="Group")
	void ResetGroupRuntimeState();

	UPROPERTY(EditAnywhere, Category="Group")
	float GroupEmotionThreshold = 0.5f;

	UPROPERTY(EditAnywhere, Category="Group|Mission", meta=(ClampMin="1.0"))
	float ReconObservationArrivalRadius = 150.f;

	FString BuildGroupLLMPrompt() const;

private:
	FName GetCommandGroupId() const;
	void ClearMissionForAllMembers();
	void BeginReconMonitoring(
		const FMissionContext& Mission,
		const FGuid& RunId,
		UScenarioExecutionLogSubsystem* ExecutionLog);
	void UpdateReconExecution();
	bool SubmitReconReportAndComplete(double ObservedAtSeconds);
	void StopReconMonitoring();
	void ForceClearCurrentCommand();

	TArray<FGroupMemoryEvent> GroupMemories;
	float AccumulatedEmotionScore = 0.f;

	UPROPERTY(Transient)
	FCommandIntent CurrentCommand;

	UPROPERTY(Transient)
	bool bHasCurrentCommand = false;

	TArray<TWeakObjectPtr<UNPCDecisionComponent>> ActiveMissionRecipients;
	FMissionContext ActiveReconMission;
	FGuid ActiveReconRunId;
	TWeakObjectPtr<UScenarioExecutionLogSubsystem> ActiveReconExecutionLog;
	double ReconExecutionStartedAtSeconds = 0.0;
	double ReconObservationStartedAtSeconds = -1.0;
	bool bReconMonitoringActive = false;
};
