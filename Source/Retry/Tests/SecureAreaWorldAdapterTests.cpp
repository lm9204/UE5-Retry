#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AI/ObjectiveAreaActor.h"
#include "AI/SecureAreaWorldAdapter.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SecureAreaWorldAdapterTests
{
	class FScopedTestWorld
	{
	public:
		FScopedTestWorld()
		{
			const FName WorldName = MakeUniqueObjectName(
				nullptr, UWorld::StaticClass(), TEXT("SecureAreaWorld"));
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

		AObjectiveAreaActor* SpawnObjective(
			const FName MarkerId, const FVector Location) const
		{
			AObjectiveAreaActor* Objective =
				World->SpawnActor<AObjectiveAreaActor>();
			Objective->MarkerId = MarkerId;
			Objective->SetActorLocation(Location);
			Objective->AreaRadius = 600.f;
			return Objective;
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
		Command.Verb = ECommandVerb::Secure;
		Command.TargetType = ECommandTargetType::Area;
		Command.TargetId = TEXT("ReconArea_A");
		Command.Status = ECommandStatus::Assigned;
		return Command;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSecureAreaWorldAdapterResolvesProjectedObjective,
	"Retry.Mission.SecureWorldAdapter.ResolvesProjectedObjective",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSecureAreaWorldAdapterResolvesProjectedObjective::RunTest(
	const FString& Parameters)
{
	SecureAreaWorldAdapterTests::FScopedTestWorld TestWorld;
	TestWorld.SpawnObjective(
		TEXT("ReconArea_A"), FVector(100.f, 200.f, 500.f));
	const FSecureAreaWorldResult Result =
		FSecureAreaWorldAdapter::ResolveWithEvaluators(
			TestWorld.GetWorld(),
			SecureAreaWorldAdapterTests::MakeCommand(),
			[](const FVector& Source, FVector& Projected)
			{
				Projected = FVector(Source.X, Source.Y, 25.f);
				return true;
			},
			[](const FVector&) { return true; });

	TestTrue(TEXT("Secure Area resolves."), Result.IsSuccess());
	TestEqual(TEXT("The Nav-projected location becomes the Mission target."),
		Result.Resolution.Mission.ObjectiveLocation,
		FVector(100.f, 200.f, 25.f));
	TestEqual(TEXT("The authored Area radius is preserved."),
		Result.AreaRadius, 600.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSecureAreaWorldAdapterRejectsWorldFailures,
	"Retry.Mission.SecureWorldAdapter.RejectsWorldFailures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSecureAreaWorldAdapterRejectsWorldFailures::RunTest(
	const FString& Parameters)
{
	auto Project = [](const FVector& Source, FVector& Result)
	{
		Result = Source;
		return true;
	};
	auto Reachable = [](const FVector&) { return true; };
	SecureAreaWorldAdapterTests::FScopedTestWorld MissingWorld;
	TestEqual(TEXT("A missing Objective is distinguished."),
		FSecureAreaWorldAdapter::ResolveWithEvaluators(
			MissingWorld.GetWorld(),
			SecureAreaWorldAdapterTests::MakeCommand(),
			Project, Reachable).Outcome,
		ESecureAreaWorldOutcome::ObjectiveNotFound);
	TestEqual(TEXT("A World without navigation is distinguished."),
		FSecureAreaWorldAdapter::Resolve(
			MissingWorld.GetWorld(),
			SecureAreaWorldAdapterTests::MakeCommand(),
			FVector::ZeroVector).Outcome,
		ESecureAreaWorldOutcome::NavigationUnavailable);

	SecureAreaWorldAdapterTests::FScopedTestWorld DuplicateWorld;
	DuplicateWorld.SpawnObjective(TEXT("ReconArea_A"), FVector::ZeroVector);
	DuplicateWorld.SpawnObjective(TEXT("ReconArea_A"), FVector(100.f, 0.f, 0.f));
	TestEqual(TEXT("Duplicate semantic Objectives are distinguished."),
		FSecureAreaWorldAdapter::ResolveWithEvaluators(
			DuplicateWorld.GetWorld(),
			SecureAreaWorldAdapterTests::MakeCommand(),
			Project, Reachable).Outcome,
		ESecureAreaWorldOutcome::DuplicateObjective);

	SecureAreaWorldAdapterTests::FScopedTestWorld ProjectionWorld;
	ProjectionWorld.SpawnObjective(TEXT("ReconArea_A"), FVector::ZeroVector);
	TestEqual(TEXT("A failed Nav projection is distinguished."),
		FSecureAreaWorldAdapter::ResolveWithEvaluators(
			ProjectionWorld.GetWorld(),
			SecureAreaWorldAdapterTests::MakeCommand(),
			[](const FVector&, FVector&) { return false; },
			Reachable).Outcome,
		ESecureAreaWorldOutcome::ObjectiveProjectionFailed);

	SecureAreaWorldAdapterTests::FScopedTestWorld BlockedWorld;
	BlockedWorld.SpawnObjective(TEXT("ReconArea_A"), FVector::ZeroVector);
	TestEqual(TEXT("A blocked path is distinguished."),
		FSecureAreaWorldAdapter::ResolveWithEvaluators(
			BlockedWorld.GetWorld(),
			SecureAreaWorldAdapterTests::MakeCommand(),
			Project, [](const FVector&) { return false; }).Outcome,
		ESecureAreaWorldOutcome::PathUnavailable);

	SecureAreaWorldAdapterTests::FScopedTestWorld InvalidRadiusWorld;
	AObjectiveAreaActor* InvalidObjective =
		InvalidRadiusWorld.SpawnObjective(
			TEXT("ReconArea_A"), FVector::ZeroVector);
	InvalidObjective->AreaRadius = 0.f;
	TestEqual(TEXT("An invalid authored radius is distinguished."),
		FSecureAreaWorldAdapter::ResolveWithEvaluators(
			InvalidRadiusWorld.GetWorld(),
			SecureAreaWorldAdapterTests::MakeCommand(),
			Project, Reachable).Outcome,
		ESecureAreaWorldOutcome::InvalidAreaRadius);
	return true;
}

#endif
