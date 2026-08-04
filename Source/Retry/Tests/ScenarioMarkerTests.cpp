#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AI/ObjectiveAreaActor.h"
#include "AI/ObservationPointActor.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ScenarioMarkerTests
{
	class FScopedTestWorld
	{
	public:
		FScopedTestWorld()
		{
			const FName WorldName = MakeUniqueObjectName(
				nullptr, UWorld::StaticClass(), TEXT("ScenarioMarkerTestWorld"));
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
		TActor* SpawnMarker(const FName MarkerId) const
		{
			TActor* Marker = World->SpawnActor<TActor>();
			Marker->MarkerId = MarkerId;
			return Marker;
		}

	private:
		UWorld* World = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioMarkersAcceptLinkedObjectiveAndObservation,
	"Retry.Scenario.Markers.AcceptsLinkedObjectiveAndObservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioMarkersAcceptLinkedObjectiveAndObservation::RunTest(
	const FString& Parameters)
{
	ScenarioMarkerTests::FScopedTestWorld TestWorld;
	AObjectiveAreaActor* Objective =
		TestWorld.SpawnMarker<AObjectiveAreaActor>(TEXT("WarehouseArea"));
	AObservationPointActor* Observation =
		TestWorld.SpawnMarker<AObservationPointActor>(TEXT("WarehouseHill"));
	Observation->ObjectiveId = Objective->MarkerId;

	const FScenarioMarkerValidationResult Result =
		AScenarioMarkerActor::ValidateMarkerSet({ Objective, Observation });
	TestTrue(TEXT("A linked Objective and Observation set is valid."),
		Result.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioMarkersRejectDuplicateIds,
	"Retry.Scenario.Markers.RejectsDuplicateIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioMarkersRejectDuplicateIds::RunTest(const FString& Parameters)
{
	ScenarioMarkerTests::FScopedTestWorld TestWorld;
	AObjectiveAreaActor* Objective =
		TestWorld.SpawnMarker<AObjectiveAreaActor>(TEXT("SharedId"));
	AObservationPointActor* Observation =
		TestWorld.SpawnMarker<AObservationPointActor>(TEXT("SharedId"));
	Observation->ObjectiveId = Objective->MarkerId;

	const FScenarioMarkerValidationResult Result =
		AScenarioMarkerActor::ValidateMarkerSet({ Objective, Observation });
	TestTrue(TEXT("Marker IDs must be unique across marker types."),
		Result.HasError(
			EScenarioMarkerValidationErrorCode::DuplicateMarkerId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenarioMarkersRejectInvalidAreaAndUnknownObjective,
	"Retry.Scenario.Markers.RejectsInvalidAreaAndUnknownObjective",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioMarkersRejectInvalidAreaAndUnknownObjective::RunTest(
	const FString& Parameters)
{
	ScenarioMarkerTests::FScopedTestWorld TestWorld;
	AObjectiveAreaActor* Objective =
		TestWorld.SpawnMarker<AObjectiveAreaActor>(TEXT("WarehouseArea"));
	Objective->AreaRadius = 0.f;
	AObservationPointActor* Observation =
		TestWorld.SpawnMarker<AObservationPointActor>(TEXT("WarehouseHill"));
	Observation->ObjectiveId = TEXT("MissingArea");

	const FScenarioMarkerValidationResult Result =
		AScenarioMarkerActor::ValidateMarkerSet({ Objective, Observation });
	TestTrue(TEXT("Objective radius must be positive."),
		Result.HasError(
			EScenarioMarkerValidationErrorCode::InvalidAreaRadius));
	TestTrue(TEXT("Observation must reference an existing Objective."),
		Result.HasError(
			EScenarioMarkerValidationErrorCode::UnknownObjectiveId));
	return true;
}

#endif
