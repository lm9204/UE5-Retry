#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AI/ObjectiveAreaActor.h"
#include "AI/ObservationPointActor.h"
#include "AI/ReconMissionWorldAdapter.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ReconMissionWorldAdapterTests
{
	class FScopedTestWorld
	{
	public:
		FScopedTestWorld()
		{
			const FName WorldName = MakeUniqueObjectName(
				nullptr,
				UWorld::StaticClass(),
				TEXT("ReconMissionWorldAdapterTestWorld"));
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

		template<typename TActor>
		TActor* SpawnMarker(
			const FName MarkerId,
			const FVector Location = FVector::ZeroVector) const
		{
			TActor* Marker = World->SpawnActor<TActor>();
			Marker->MarkerId = MarkerId;
			Marker->SetActorLocation(Location);
			return Marker;
		}

		UWorld* GetWorld() const
		{
			return World;
		}

	private:
		UWorld* World = nullptr;
	};

	FCommandIntent MakeAssignedReconCommand()
	{
		FCommandIntent Command;
		Command.CommandId = FGuid::NewGuid();
		Command.IssuerId = TEXT("HQ");
		Command.AssignedGroupId = TEXT("ReconGroup");
		Command.Verb = ECommandVerb::Recon;
		Command.TargetType = ECommandTargetType::Area;
		Command.TargetId = TEXT("ReconArea_A");
		Command.Status = ECommandStatus::Assigned;
		return Command;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FReconMissionWorldAdapterProjectsMarkerToNavigationHeight,
	"Retry.Mission.WorldAdapter.ProjectsMarkerToNavigationHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReconMissionWorldAdapterProjectsMarkerToNavigationHeight::RunTest(
	const FString& Parameters)
{
	ReconMissionWorldAdapterTests::FScopedTestWorld TestWorld;
	AObjectiveAreaActor* Objective =
		TestWorld.SpawnMarker<AObjectiveAreaActor>(TEXT("ReconArea_A"));
	AObservationPointActor* Observation =
		TestWorld.SpawnMarker<AObservationPointActor>(
			TEXT("ReconObs_A1"), FVector(100.f, 0.f, 500.f));
	Observation->ObjectiveId = Objective->MarkerId;

	const FReconMissionWorldResult Result =
		FReconMissionWorldAdapter::ResolveWithEvaluators(
			TestWorld.GetWorld(),
			ReconMissionWorldAdapterTests::MakeAssignedReconCommand(),
			[](const FVector& MarkerLocation, FVector& OutMovementLocation)
			{
				OutMovementLocation = FVector(
					MarkerLocation.X, MarkerLocation.Y, 25.f);
				return true;
			},
			[](const FVector&, double& OutPathLength)
			{
				OutPathLength = 100.0;
				return true;
			});

	TestTrue(TEXT("Projected observation resolves into a Mission."),
		Result.IsSuccess());
	TestEqual(TEXT("Mission movement uses the projected Nav height."),
		Result.Resolution.Mission.ObjectiveLocation,
		FVector(100.f, 0.f, 25.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FReconMissionWorldAdapterSelectsShortestReachablePath,
	"Retry.Mission.WorldAdapter.SelectsShortestReachablePath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReconMissionWorldAdapterSelectsShortestReachablePath::RunTest(
	const FString& Parameters)
{
	ReconMissionWorldAdapterTests::FScopedTestWorld TestWorld;
	AObjectiveAreaActor* Objective =
		TestWorld.SpawnMarker<AObjectiveAreaActor>(TEXT("ReconArea_A"));
	AObservationPointActor* First =
		TestWorld.SpawnMarker<AObservationPointActor>(
			TEXT("ReconObs_A1"), FVector(100.f, 0.f, 0.f));
	AObservationPointActor* Second =
		TestWorld.SpawnMarker<AObservationPointActor>(
			TEXT("ReconObs_A2"), FVector(200.f, 0.f, 0.f));
	First->ObjectiveId = Objective->MarkerId;
	Second->ObjectiveId = Objective->MarkerId;

	const FReconMissionWorldResult Result =
		FReconMissionWorldAdapter::ResolveWithPathEvaluator(
			TestWorld.GetWorld(),
			ReconMissionWorldAdapterTests::MakeAssignedReconCommand(),
			[](const FVector& Destination, double& OutPathLength)
			{
				OutPathLength = Destination.X == 100.f ? 500.0 : 200.0;
				return true;
			});

	TestTrue(TEXT("World markers resolve into a Mission."),
		Result.IsSuccess());
	TestEqual(TEXT("Both linked observations are collected."),
		Result.CandidateCount, 2);
	TestEqual(TEXT("The shortest reachable Nav path wins."),
		Result.Resolution.Mission.ObjectiveId,
		FName(TEXT("ReconObs_A2")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FReconMissionWorldAdapterReportsObjectiveFailures,
	"Retry.Mission.WorldAdapter.ReportsObjectiveFailures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReconMissionWorldAdapterReportsObjectiveFailures::RunTest(
	const FString& Parameters)
{
	auto AlwaysReachable = [](const FVector&, double& OutPathLength)
	{
		OutPathLength = 100.0;
		return true;
	};

	ReconMissionWorldAdapterTests::FScopedTestWorld MissingWorld;
	const FReconMissionWorldResult Missing =
		FReconMissionWorldAdapter::ResolveWithPathEvaluator(
			MissingWorld.GetWorld(),
			ReconMissionWorldAdapterTests::MakeAssignedReconCommand(),
			AlwaysReachable);
	TestEqual(TEXT("A missing Objective is distinguished."),
		Missing.Outcome,
		EReconMissionWorldOutcome::ObjectiveNotFound);

	ReconMissionWorldAdapterTests::FScopedTestWorld DuplicateWorld;
	DuplicateWorld.SpawnMarker<AObjectiveAreaActor>(TEXT("ReconArea_A"));
	DuplicateWorld.SpawnMarker<AObjectiveAreaActor>(TEXT("ReconArea_A"));
	const FReconMissionWorldResult Duplicate =
		FReconMissionWorldAdapter::ResolveWithPathEvaluator(
			DuplicateWorld.GetWorld(),
			ReconMissionWorldAdapterTests::MakeAssignedReconCommand(),
			AlwaysReachable);
	TestEqual(TEXT("Duplicate Objectives are distinguished."),
		Duplicate.Outcome,
		EReconMissionWorldOutcome::DuplicateObjective);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FReconMissionWorldAdapterPreservesNoReachableCandidate,
	"Retry.Mission.WorldAdapter.PreservesNoReachableCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReconMissionWorldAdapterPreservesNoReachableCandidate::RunTest(
	const FString& Parameters)
{
	ReconMissionWorldAdapterTests::FScopedTestWorld TestWorld;
	AObjectiveAreaActor* Objective =
		TestWorld.SpawnMarker<AObjectiveAreaActor>(TEXT("ReconArea_A"));
	AObservationPointActor* Observation =
		TestWorld.SpawnMarker<AObservationPointActor>(
			TEXT("ReconObs_A1"), FVector(100.f, 0.f, 0.f));
	Observation->ObjectiveId = Objective->MarkerId;

	const FReconMissionWorldResult Result =
		FReconMissionWorldAdapter::ResolveWithPathEvaluator(
			TestWorld.GetWorld(),
			ReconMissionWorldAdapterTests::MakeAssignedReconCommand(),
			[](const FVector&, double&)
			{
				return false;
			});

	TestEqual(TEXT("World resolution reports Mission failure."),
		Result.Outcome,
		EReconMissionWorldOutcome::MissionResolutionFailed);
	TestEqual(TEXT("No reachable candidate remains distinguishable."),
		Result.Resolution.SelectionOutcome,
		EObservationPointSelectionOutcome::NoUsableCandidates);
	return true;
}

#endif
