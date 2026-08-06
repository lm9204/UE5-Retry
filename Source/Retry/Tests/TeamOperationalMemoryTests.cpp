#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AI/TeamOperationalMemorySubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace TeamOperationalMemoryTests
{
	class FScopedTestWorld
	{
	public:
		FScopedTestWorld()
		{
			const FName WorldName = MakeUniqueObjectName(
				nullptr, UWorld::StaticClass(), TEXT("TeamOperationalMemoryWorld"));
			World = NewObject<UWorld>(GetTransientPackage(), WorldName);
			// UWorldSubsystem follows runtime world support rules. EditorPreview
			// intentionally does not create it, so exercise the subsystem in the
			// same world type used by the game.
			World->WorldType = EWorldType::Game;
			FWorldContext& Context =
				GEngine->CreateNewWorldContext(World->WorldType);
			Context.SetCurrentWorld(World);
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

		UTeamOperationalMemorySubsystem* GetMemory() const
		{
			return World->GetSubsystem<UTeamOperationalMemorySubsystem>();
		}

	private:
		UWorld* World = nullptr;
	};

	FOperationalReport MakeReport(
		const uint8 TeamId,
		const FGuid& RunId,
		const FGuid& CommandId)
	{
		FOperationalReport Report;
		Report.ReportId = FGuid::NewGuid();
		Report.RunId = RunId;
		Report.CommandId = CommandId;
		Report.TeamId = TeamId;
		Report.SourceGroupId = TEXT("A");
		Report.CreatedAtSeconds = 0.0;

		FOperationalFact& Fact = Report.Facts.AddDefaulted_GetRef();
		Fact.FactId = FGuid::NewGuid();
		Fact.RunId = RunId;
		Fact.CommandId = CommandId;
		Fact.TeamId = TeamId;
		Fact.SourceGroupId = TEXT("A");
		Fact.PredicateId = TEXT("AreaObserved");
		Fact.SubjectId = TEXT("ReconArea_A");
		return Report;
	}

	FInformationRequirement MakeRequirement()
	{
		FInformationRequirement Requirement;
		Requirement.RequirementId = TEXT("AreaObserved");
		Requirement.SubjectId = TEXT("ReconArea_A");
		return Requirement;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTeamOperationalMemoryGatesFactsOnReceive,
	"Retry.Operational.TeamMemory.GatesFactsOnReceive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTeamOperationalMemoryGatesFactsOnReceive::RunTest(
	const FString& Parameters)
{
	TeamOperationalMemoryTests::FScopedTestWorld TestWorld;
	UTeamOperationalMemorySubsystem* Memory = TestWorld.GetMemory();
	TestNotNull(TEXT("World creates Team Operational Memory."), Memory);
	if (!Memory)
	{
		return false;
	}
	const FGuid RunId = FGuid::NewGuid();
	const FGuid CommandId = FGuid::NewGuid();
	const FOperationalReport Report =
		TeamOperationalMemoryTests::MakeReport(1, RunId, CommandId);
	const FInformationRequirement Requirement =
		TeamOperationalMemoryTests::MakeRequirement();

	TestFalse(TEXT("Created report does not update Team Memory."),
		Memory->HasReceivedRequirement(1, RunId, CommandId, Requirement));
	FOperationalReport Received;
	FText Error;
	TestTrue(TEXT("Valid report is received."),
		Memory->ReceiveReport(Report, Received, Error));
	TestEqual(TEXT("Received status is explicit."),
		Received.Status, EOperationalReportStatus::Received);
	TestTrue(TEXT("Received Fact satisfies the requirement."),
		Memory->HasReceivedRequirement(1, RunId, CommandId, Requirement));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTeamOperationalMemoryPartitionsAndDeduplicates,
	"Retry.Operational.TeamMemory.PartitionsAndDeduplicates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTeamOperationalMemoryPartitionsAndDeduplicates::RunTest(
	const FString& Parameters)
{
	TeamOperationalMemoryTests::FScopedTestWorld TestWorld;
	UTeamOperationalMemorySubsystem* Memory = TestWorld.GetMemory();
	TestNotNull(TEXT("World creates Team Operational Memory."), Memory);
	if (!Memory)
	{
		return false;
	}
	const FOperationalReport Report =
		TeamOperationalMemoryTests::MakeReport(
			1, FGuid::NewGuid(), FGuid::NewGuid());
	FOperationalReport Received;
	FText Error;
	TestTrue(TEXT("First delivery succeeds."),
		Memory->ReceiveReport(Report, Received, Error));
	TestTrue(TEXT("Duplicate delivery is idempotent."),
		Memory->ReceiveReport(Report, Received, Error));
	FOperationalReport Conflicting = Report;
	Conflicting.CommandId = FGuid::NewGuid();
	Conflicting.Facts[0].CommandId = Conflicting.CommandId;
	TestFalse(TEXT("The same Report ID cannot identify different data."),
		Memory->ReceiveReport(Conflicting, Received, Error));
	TestEqual(TEXT("Team 1 stores one Fact."),
		Memory->GetFactsForTeam(1).Num(), 1);
	TestEqual(TEXT("Another Team sees no Fact."),
		Memory->GetFactsForTeam(2).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTeamOperationalMemoryResetsRunState,
	"Retry.Operational.TeamMemory.ResetsRunState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTeamOperationalMemoryResetsRunState::RunTest(
	const FString& Parameters)
{
	TeamOperationalMemoryTests::FScopedTestWorld TestWorld;
	UTeamOperationalMemorySubsystem* Memory = TestWorld.GetMemory();
	TestNotNull(TEXT("World creates Team Operational Memory."), Memory);
	if (!Memory)
	{
		return false;
	}
	const FOperationalReport Report =
		TeamOperationalMemoryTests::MakeReport(
			1, FGuid::NewGuid(), FGuid::NewGuid());
	FOperationalReport Received;
	FText Error;
	Memory->ReceiveReport(Report, Received, Error);
	Memory->ResetOperationalMemory();
	TestTrue(TEXT("Reset clears Facts."),
		Memory->GetFactsForTeam(1).IsEmpty());
	TestTrue(TEXT("Reset clears Reports."),
		Memory->GetReceivedReportsForTeam(1).IsEmpty());
	return true;
}

#endif
