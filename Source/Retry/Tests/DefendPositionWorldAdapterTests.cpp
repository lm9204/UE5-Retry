#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AI/DefendPositionWorldAdapter.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace DefendPositionWorldAdapterTests
{
	class FScopedTestWorld
	{
	public:
		FScopedTestWorld()
		{
			const FName WorldName = MakeUniqueObjectName(
				nullptr, UWorld::StaticClass(), TEXT("DefendPositionWorld"));
			World = NewObject<UWorld>(GetTransientPackage(), WorldName);
			World->WorldType = EWorldType::EditorPreview;
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

		UWorld* GetWorld() const { return World; }

	private:
		UWorld* World = nullptr;
	};

	FCommandIntent MakeCommand()
	{
		FCommandIntent Command;
		Command.CommandId = FGuid::NewGuid();
		Command.IssuerId = TEXT("HQ");
		Command.AssignedGroupId = TEXT("A");
		Command.Verb = ECommandVerb::Defend;
		Command.TargetType = ECommandTargetType::Position;
		Command.TargetId = TEXT("ReconArea_A");
		Command.TargetLocation = FVector(100.f, 200.f, 500.f);
		Command.Status = ECommandStatus::Assigned;
		return Command;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDefendPositionWorldAdapterResolvesProjectedTarget,
	"Retry.Mission.DefendWorldAdapter.ResolvesProjectedTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDefendPositionWorldAdapterResolvesProjectedTarget::RunTest(
	const FString& Parameters)
{
	DefendPositionWorldAdapterTests::FScopedTestWorld TestWorld;
	const FDefendPositionWorldResult Result =
		FDefendPositionWorldAdapter::ResolveWithEvaluators(
			TestWorld.GetWorld(),
			DefendPositionWorldAdapterTests::MakeCommand(),
			[](const FVector& Source, FVector& Projected)
			{
				Projected = FVector(Source.X, Source.Y, 25.f);
				return true;
			},
			[](const FVector&) { return true; });
	TestTrue(TEXT("Defend Position resolves."), Result.IsSuccess());
	TestEqual(TEXT("Nav projection supplies the movement height."),
		Result.Resolution.Mission.ObjectiveLocation,
		FVector(100.f, 200.f, 25.f));
	TestEqual(TEXT("The secured Area remains the semantic subject."),
		Result.Resolution.Mission.ObjectiveId,
		FName(TEXT("ReconArea_A")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDefendPositionWorldAdapterRejectsProjectionAndPathFailures,
	"Retry.Mission.DefendWorldAdapter.RejectsWorldFailures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDefendPositionWorldAdapterRejectsProjectionAndPathFailures::RunTest(
	const FString& Parameters)
{
	DefendPositionWorldAdapterTests::FScopedTestWorld TestWorld;
	auto Project = [](const FVector& Source, FVector& Result)
	{
		Result = Source;
		return true;
	};
	TestEqual(TEXT("Projection failure is distinguished."),
		FDefendPositionWorldAdapter::ResolveWithEvaluators(
			TestWorld.GetWorld(),
			DefendPositionWorldAdapterTests::MakeCommand(),
			[](const FVector&, FVector&) { return false; },
			[](const FVector&) { return true; }).Outcome,
		EDefendPositionWorldOutcome::TargetProjectionFailed);
	TestEqual(TEXT("Path failure is distinguished."),
		FDefendPositionWorldAdapter::ResolveWithEvaluators(
			TestWorld.GetWorld(),
			DefendPositionWorldAdapterTests::MakeCommand(),
			Project,
			[](const FVector&) { return false; }).Outcome,
		EDefendPositionWorldOutcome::PathUnavailable);
	return true;
}

#endif
