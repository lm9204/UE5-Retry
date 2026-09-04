#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "AI/GroupManagerActor.h"
#include "Components/NPCDecisionComponent.h"
#include "Scenario/ScenarioExecutionLogSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GroupMissionDispatchTests
{
	class FScopedTestWorld
	{
	public:
		FScopedTestWorld()
		{
			const FName WorldName = MakeUniqueObjectName(
				nullptr, UWorld::StaticClass(), TEXT("GroupMissionDispatchWorld"));
			World = NewObject<UWorld>(GetTransientPackage(), WorldName);
			World->WorldType = EWorldType::EditorPreview;

			FWorldContext& WorldContext =
				GEngine->CreateNewWorldContext(World->WorldType);
			WorldContext.SetCurrentWorld(World);
			World->AddToRoot();
			World->InitializeNewWorld(UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.CreatePhysicsScene(false)
				.RequiresHitProxies(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false));
		}

		~FScopedTestWorld()
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}

		AGroupManagerActor* SpawnGroup(const FString& GroupId) const
		{
			AGroupManagerActor* Group =
				World->SpawnActor<AGroupManagerActor>();
			Group->GroupID = GroupId;
			return Group;
		}

	private:
		UWorld* World = nullptr;
	};

	struct FExecutionLogFixture
	{
		FScenarioRunContext RunContext;
		UScenarioExecutionLogSubsystem* Log = nullptr;

		FExecutionLogFixture()
		{
			RunContext.RunId = FGuid::NewGuid();
			RunContext.ScenarioId = TEXT("GroupMissionDispatchTest");
			RunContext.Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(
				TEXT("/Game/Tests/DummyScenarioLevel.DummyScenarioLevel")));
			RunContext.bIsActive = true;

			UGameInstance* GameInstance = NewObject<UGameInstance>();
			Log = NewObject<UScenarioExecutionLogSubsystem>(GameInstance);
			Log->StartRun(RunContext);
		}
	};

	FCommandIntent MakeCommand()
	{
		FCommandIntent Command;
		Command.CommandId = FGuid::NewGuid();
		Command.IssuerId = TEXT("HQ");
		Command.AssignedGroupId = TEXT("GroupA");
		Command.Verb = ECommandVerb::Recon;
		Command.TargetType = ECommandTargetType::Area;
		Command.TargetId = TEXT("ObjectiveA");
		return Command;
	}

	FMissionContext MakeMission(const FGuid& CommandId, const FName PointId)
	{
		FMissionContext Mission;
		Mission.CommandId = CommandId;
		Mission.ObjectiveId = PointId;
		Mission.ObjectiveLocation = FVector(100.0, 200.0, 0.0);
		return Mission;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGroupMissionDispatchAppliesToAllBeforeExecuting,
	"Retry.Mission.GroupDispatch.AppliesToAllBeforeExecuting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroupMissionDispatchAppliesToAllBeforeExecuting::RunTest(
	const FString& Parameters)
{
	GroupMissionDispatchTests::FScopedTestWorld TestWorld;
	GroupMissionDispatchTests::FExecutionLogFixture LogFixture;
	AGroupManagerActor* Group = TestWorld.SpawnGroup(TEXT("GroupA"));
	const FCommandIntent Command = GroupMissionDispatchTests::MakeCommand();
	TestTrue(TEXT("Command is assigned before dispatch."),
		Group->AssignCommandForRun(
			Command, LogFixture.RunContext.RunId, LogFixture.Log).IsSuccess());

	UNPCDecisionComponent* LeaderDecision =
		NewObject<UNPCDecisionComponent>();
	UNPCDecisionComponent* MemberDecision =
		NewObject<UNPCDecisionComponent>();
	const FMissionContext Mission =
		GroupMissionDispatchTests::MakeMission(
			Command.CommandId, TEXT("ObservationA"));
	const FGroupMissionDispatchResult Result =
		Group->DispatchResolvedMissionForRun(
			Mission,
			{LeaderDecision, MemberDecision},
			LogFixture.RunContext.RunId,
			LogFixture.Log);

	TestTrue(TEXT("Atomic dispatch succeeds."), Result.IsSuccess());
	TestEqual(TEXT("Both recipients are counted."), Result.RecipientCount, 2);
	TestTrue(TEXT("Leader owns the resolved Mission."),
		LeaderDecision->HasActiveMission());
	TestTrue(TEXT("Member owns the resolved Mission."),
		MemberDecision->HasActiveMission());
	TestEqual(TEXT("Group advances only after fan-out succeeds."),
		Group->GetCurrentCommand().Status, ECommandStatus::Executing);
	TestEqual(TEXT("Execution transition is recorded after assignment."),
		LogFixture.Log->GetActiveRunLog().Events.Last().NewStatus,
		ECommandStatus::Executing);
	TestTrue(TEXT("Executing command can complete."),
		Group->TransitionCurrentCommandStatusForRun(
			ECommandStatus::Completed,
			TEXT("TestCompleted"),
			TEXT("Completed by automation test."),
			LogFixture.RunContext.RunId,
			LogFixture.Log));
	TestFalse(TEXT("Terminal transition clears the leader Mission."),
		LeaderDecision->HasActiveMission());
	TestFalse(TEXT("Terminal transition clears the member Mission."),
		MemberDecision->HasActiveMission());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGroupMissionDispatchStartsSecureAreaExecution,
	"Retry.Mission.GroupDispatch.StartsSecureAreaExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroupMissionDispatchStartsSecureAreaExecution::RunTest(
	const FString& Parameters)
{
	GroupMissionDispatchTests::FScopedTestWorld TestWorld;
	GroupMissionDispatchTests::FExecutionLogFixture LogFixture;
	AGroupManagerActor* Group = TestWorld.SpawnGroup(TEXT("GroupA"));
	FCommandIntent Command = GroupMissionDispatchTests::MakeCommand();
	Command.Verb = ECommandVerb::Secure;
	TestTrue(TEXT("Secure command is assigned before dispatch."),
		Group->AssignCommandForRun(
			Command, LogFixture.RunContext.RunId, LogFixture.Log).IsSuccess());

	UNPCDecisionComponent* LeaderDecision =
		NewObject<UNPCDecisionComponent>();
	UNPCDecisionComponent* MemberDecision =
		NewObject<UNPCDecisionComponent>();
	const FMissionContext Mission =
		GroupMissionDispatchTests::MakeMission(
			Command.CommandId, TEXT("ObjectiveA"));
	const FGroupMissionDispatchResult InvalidRadiusResult =
		Group->DispatchResolvedMissionForRun(
			Mission,
			{LeaderDecision, MemberDecision},
			LogFixture.RunContext.RunId,
			LogFixture.Log,
			0.f);
	TestEqual(TEXT("Secure dispatch rejects a missing Area radius."),
		InvalidRadiusResult.Outcome,
		EGroupMissionDispatchOutcome::MissionRejected);
	TestFalse(TEXT("Radius preflight does not mutate recipients."),
		LeaderDecision->HasActiveMission()
		|| MemberDecision->HasActiveMission());

	const FGroupMissionDispatchResult Result =
		Group->DispatchResolvedMissionForRun(
			Mission,
			{LeaderDecision, MemberDecision},
			LogFixture.RunContext.RunId,
			LogFixture.Log,
			500.f);

	TestTrue(TEXT("Secure Area dispatch succeeds."), Result.IsSuccess());
	TestEqual(TEXT("Secure command begins execution."),
		Group->GetCurrentCommand().Status, ECommandStatus::Executing);
	TestTrue(TEXT("Every recipient receives the Area Mission."),
		LeaderDecision->HasActiveMission()
		&& MemberDecision->HasActiveMission());
	TestTrue(TEXT("Secure command can terminate cleanly."),
		Group->TransitionCurrentCommandStatusForRun(
			ECommandStatus::Cancelled,
			TEXT("TestCancelled"),
			TEXT("Cancelled by automation test."),
			LogFixture.RunContext.RunId,
			LogFixture.Log));
	TestFalse(TEXT("Terminal cleanup clears the leader Mission."),
		LeaderDecision->HasActiveMission());
	TestFalse(TEXT("Terminal cleanup clears the member Mission."),
		MemberDecision->HasActiveMission());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGroupMissionDispatchKeepsDefendMissionExecuting,
	"Retry.Mission.GroupDispatch.KeepsDefendMissionExecuting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroupMissionDispatchKeepsDefendMissionExecuting::RunTest(
	const FString& Parameters)
{
	GroupMissionDispatchTests::FScopedTestWorld TestWorld;
	GroupMissionDispatchTests::FExecutionLogFixture LogFixture;
	AGroupManagerActor* Group = TestWorld.SpawnGroup(TEXT("GroupA"));
	FCommandIntent Command = GroupMissionDispatchTests::MakeCommand();
	Command.Verb = ECommandVerb::Defend;
	Command.TargetType = ECommandTargetType::Position;
	Command.TargetLocation = FVector(100.f, 200.f, 0.f);
	TestTrue(TEXT("Defend command is assigned before dispatch."),
		Group->AssignCommandForRun(
			Command, LogFixture.RunContext.RunId, LogFixture.Log).IsSuccess());

	UNPCDecisionComponent* LeaderDecision =
		NewObject<UNPCDecisionComponent>();
	UNPCDecisionComponent* MemberDecision =
		NewObject<UNPCDecisionComponent>();
	const FMissionContext Mission =
		GroupMissionDispatchTests::MakeMission(
			Command.CommandId, TEXT("ObjectiveA"));
	const FGroupMissionDispatchResult Result =
		Group->DispatchResolvedMissionForRun(
			Mission,
			{LeaderDecision, MemberDecision},
			LogFixture.RunContext.RunId,
			LogFixture.Log);

	TestTrue(TEXT("Defend Mission dispatch succeeds."), Result.IsSuccess());
	TestEqual(TEXT("Defend remains an executing persistent command."),
		Group->GetCurrentCommand().Status, ECommandStatus::Executing);
	TestTrue(TEXT("Every recipient keeps the Defend Mission."),
		LeaderDecision->HasActiveMission()
		&& MemberDecision->HasActiveMission());
	TestTrue(TEXT("A later replan can cancel the persistent Mission."),
		Group->TransitionCurrentCommandStatusForRun(
			ECommandStatus::Cancelled,
			TEXT("Replanned"),
			TEXT("Cancelled by automation test."),
			LogFixture.RunContext.RunId,
			LogFixture.Log));
	TestFalse(TEXT("Cancellation clears the persistent Mission."),
		LeaderDecision->HasActiveMission()
		|| MemberDecision->HasActiveMission());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGroupMissionDispatchRestoresPreviousStateWhenTransitionFails,
	"Retry.Mission.GroupDispatch.RestoresWhenTransitionFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroupMissionDispatchRestoresPreviousStateWhenTransitionFails::RunTest(
	const FString& Parameters)
{
	GroupMissionDispatchTests::FScopedTestWorld TestWorld;
	GroupMissionDispatchTests::FExecutionLogFixture LogFixture;
	AGroupManagerActor* Group = TestWorld.SpawnGroup(TEXT("GroupA"));
	const FCommandIntent Command = GroupMissionDispatchTests::MakeCommand();
	Group->AssignCommandForRun(
		Command, LogFixture.RunContext.RunId, LogFixture.Log);

	UNPCDecisionComponent* LeaderDecision =
		NewObject<UNPCDecisionComponent>();
	UNPCDecisionComponent* MemberDecision =
		NewObject<UNPCDecisionComponent>();
	const FMissionContext PreviousMission =
		GroupMissionDispatchTests::MakeMission(
			FGuid::NewGuid(), TEXT("PreviousPoint"));
	LeaderDecision->SetMissionContext(PreviousMission);

	const FMissionContext NewMission =
		GroupMissionDispatchTests::MakeMission(
			Command.CommandId, TEXT("ObservationA"));
	const FGroupMissionDispatchResult Result =
		Group->DispatchResolvedMissionForRun(
			NewMission,
			{LeaderDecision, MemberDecision},
			FGuid::NewGuid(),
			LogFixture.Log);

	TestEqual(TEXT("Stale Run prevents the final status transition."),
		Result.Outcome,
		EGroupMissionDispatchOutcome::StatusTransitionFailed);
	TestEqual(TEXT("The leader's previous Mission is restored."),
		LeaderDecision->GetActiveMissionContext().CommandId,
		PreviousMission.CommandId);
	TestFalse(TEXT("A previously empty member is restored to empty."),
		MemberDecision->HasActiveMission());
	TestEqual(TEXT("Command remains Assigned after rollback."),
		Group->GetCurrentCommand().Status, ECommandStatus::Assigned);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGroupMissionDispatchRejectsUnavailableRecipientBeforeMutation,
	"Retry.Mission.GroupDispatch.RejectsUnavailableRecipient",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroupMissionDispatchRejectsUnavailableRecipientBeforeMutation::RunTest(
	const FString& Parameters)
{
	GroupMissionDispatchTests::FScopedTestWorld TestWorld;
	GroupMissionDispatchTests::FExecutionLogFixture LogFixture;
	AGroupManagerActor* Group = TestWorld.SpawnGroup(TEXT("GroupA"));
	const FCommandIntent Command = GroupMissionDispatchTests::MakeCommand();
	Group->AssignCommandForRun(
		Command, LogFixture.RunContext.RunId, LogFixture.Log);

	UNPCDecisionComponent* AvailableDecision =
		NewObject<UNPCDecisionComponent>();
	const FMissionContext Mission =
		GroupMissionDispatchTests::MakeMission(
			Command.CommandId, TEXT("ObservationA"));
	const FGroupMissionDispatchResult Result =
		Group->DispatchResolvedMissionForRun(
			Mission,
			{AvailableDecision, nullptr},
			LogFixture.RunContext.RunId,
			LogFixture.Log);

	TestEqual(TEXT("One unavailable recipient rejects the whole dispatch."),
		Result.Outcome,
		EGroupMissionDispatchOutcome::RecipientUnavailable);
	TestFalse(TEXT("No valid recipient is mutated during preflight failure."),
		AvailableDecision->HasActiveMission());
	TestEqual(TEXT("Command remains Assigned when fan-out is rejected."),
		Group->GetCurrentCommand().Status, ECommandStatus::Assigned);
	return true;
}

#endif
