#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "AI/GroupManagerActor.h"
#include "Scenario/ScenarioExecutionLogSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GroupCommandAuthorityTests
{
	class FScopedTestWorld
	{
	public:
		FScopedTestWorld()
		{
			const FName WorldName = MakeUniqueObjectName(
				nullptr, UWorld::StaticClass(), TEXT("GroupCommandTestWorld"));
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
			RunContext.ScenarioId = TEXT("GroupAuthorityTest");
			RunContext.Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(
				TEXT("/Game/Tests/DummyScenarioLevel.DummyScenarioLevel")));
			RunContext.bIsActive = true;

			UGameInstance* GameInstance = NewObject<UGameInstance>();
			Log = NewObject<UScenarioExecutionLogSubsystem>(GameInstance);
			Log->StartRun(RunContext);
		}
	};

	FCommandIntent MakeValidCommand(const FName GroupId)
	{
		FCommandIntent Command;
		Command.CommandId = FGuid::NewGuid();
		Command.IssuerId = TEXT("HQ");
		Command.AssignedGroupId = GroupId;
		Command.Verb = ECommandVerb::Recon;
		Command.TargetType = ECommandTargetType::Area;
		Command.TargetId = TEXT("ObjectiveA");
		return Command;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGroupCommandAuthorityAssignsAndLogs,
	"Retry.Command.GroupAuthority.AssignsAndLogs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroupCommandAuthorityAssignsAndLogs::RunTest(
	const FString& Parameters)
{
	GroupCommandAuthorityTests::FScopedTestWorld TestWorld;
	GroupCommandAuthorityTests::FExecutionLogFixture LogFixture;
	AGroupManagerActor* Group = TestWorld.SpawnGroup(TEXT("GroupA"));
	const FCommandIntent Command =
		GroupCommandAuthorityTests::MakeValidCommand(TEXT("GroupA"));

	const FCommandAssignmentResult Result = Group->AssignCommandForRun(
		Command, LogFixture.RunContext.RunId, LogFixture.Log);
	TestTrue(TEXT("Valid command is assigned."), Result.IsSuccess());
	TestTrue(TEXT("Group owns the assigned command."), Group->HasCurrentCommand());
	TestEqual(TEXT("Assigned command keeps its identity."),
		Group->GetCurrentCommand().CommandId, Command.CommandId);
	TestEqual(TEXT("Assignment advances the command to Assigned."),
		Group->GetCurrentCommand().Status, ECommandStatus::Assigned);

	const FScenarioExecutionRunLog RunLog = LogFixture.Log->GetActiveRunLog();
	TestEqual(TEXT("Run start, validation, and two transitions are logged."),
		RunLog.Events.Num(), 4);
	TestEqual(TEXT("Last transition ends at Assigned."),
		RunLog.Events.Last().NewStatus, ECommandStatus::Assigned);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGroupCommandAuthorityRejectsInvalidOrMismatchedCommands,
	"Retry.Command.GroupAuthority.RejectsInvalidOrMismatchedCommands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroupCommandAuthorityRejectsInvalidOrMismatchedCommands::RunTest(
	const FString& Parameters)
{
	GroupCommandAuthorityTests::FScopedTestWorld TestWorld;
	GroupCommandAuthorityTests::FExecutionLogFixture LogFixture;
	AGroupManagerActor* Group = TestWorld.SpawnGroup(TEXT("GroupA"));

	FCommandIntent InvalidCommand =
		GroupCommandAuthorityTests::MakeValidCommand(TEXT("GroupA"));
	InvalidCommand.TargetId = NAME_None;
	const FCommandAssignmentResult InvalidResult = Group->AssignCommandForRun(
		InvalidCommand, LogFixture.RunContext.RunId, LogFixture.Log);
	TestEqual(TEXT("Invalid content returns ValidationRejected."),
		InvalidResult.Outcome, ECommandAssignmentOutcome::ValidationRejected);
	TestTrue(TEXT("Validation details retain the missing target issue."),
		InvalidResult.Validation.HasError(
			ECommandValidationErrorCode::MissingTargetId));

	const FCommandIntent MismatchedCommand =
		GroupCommandAuthorityTests::MakeValidCommand(TEXT("GroupB"));
	const FCommandAssignmentResult MismatchResult = Group->AssignCommandForRun(
		MismatchedCommand, LogFixture.RunContext.RunId, LogFixture.Log);
	TestEqual(TEXT("Another group's command is rejected."),
		MismatchResult.Outcome, ECommandAssignmentOutcome::GroupMismatch);
	TestFalse(TEXT("Rejected commands are never owned."),
		Group->HasCurrentCommand());
	TestEqual(TEXT("Both rejections are recorded after RunStarted."),
		LogFixture.Log->GetActiveRunLog().Events.Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGroupCommandAuthorityPreventsReplacementAndRequiresTerminalClear,
	"Retry.Command.GroupAuthority.PreventsReplacementAndRequiresTerminalClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroupCommandAuthorityPreventsReplacementAndRequiresTerminalClear::RunTest(
	const FString& Parameters)
{
	GroupCommandAuthorityTests::FScopedTestWorld TestWorld;
	GroupCommandAuthorityTests::FExecutionLogFixture LogFixture;
	AGroupManagerActor* Group = TestWorld.SpawnGroup(TEXT("GroupA"));
	const FCommandIntent FirstCommand =
		GroupCommandAuthorityTests::MakeValidCommand(TEXT("GroupA"));
	const FCommandIntent SecondCommand =
		GroupCommandAuthorityTests::MakeValidCommand(TEXT("GroupA"));

	TestTrue(TEXT("First command is assigned."),
		Group->AssignCommandForRun(
			FirstCommand, LogFixture.RunContext.RunId, LogFixture.Log).IsSuccess());
	TestFalse(TEXT("A non-terminal command cannot be silently cleared."),
		Group->ClearCurrentCommand());

	const FCommandAssignmentResult ReplacementResult = Group->AssignCommandForRun(
		SecondCommand, LogFixture.RunContext.RunId, LogFixture.Log);
	TestEqual(TEXT("Replacement is rejected while a command is active."),
		ReplacementResult.Outcome,
		ECommandAssignmentOutcome::ActiveCommandExists);
	TestEqual(TEXT("The first command remains authoritative."),
		Group->GetCurrentCommand().CommandId, FirstCommand.CommandId);

	TestTrue(TEXT("Assigned command can transition to Cancelled."),
		Group->TransitionCurrentCommandStatusForRun(
			ECommandStatus::Cancelled,
			TEXT("TestCancellation"),
			TEXT("Cancelled by automation test."),
			LogFixture.RunContext.RunId,
			LogFixture.Log));
	TestTrue(TEXT("A terminal command can be cleared."),
		Group->ClearCurrentCommand());
	TestFalse(TEXT("Clear removes command ownership."),
		Group->HasCurrentCommand());
	return true;
}

#endif
