#include "AI/ScenarioMarkerActor.h"

#include "Components/SceneComponent.h"
#include "AI/ObjectiveAreaActor.h"

#define LOCTEXT_NAMESPACE "ScenarioMarkerActor"

AScenarioMarkerActor::AScenarioMarkerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

FName AScenarioMarkerActor::GetMarkerId() const
{
	return MarkerId;
}

FScenarioMarkerValidationResult AScenarioMarkerActor::ValidateMarkerSet(
	const TArray<AScenarioMarkerActor*>& Markers)
{
	FScenarioMarkerValidationResult Result;
	TSet<FName> SeenMarkerIds;
	TSet<FName> ObjectiveIds;

	for (const AScenarioMarkerActor* Marker : Markers)
	{
		if (!IsValid(Marker))
		{
			AddValidationIssue(
				Result,
				EScenarioMarkerValidationErrorCode::InvalidMarkerReference,
				NAME_None,
				LOCTEXT("InvalidMarkerReference", "Marker reference is invalid."));
			continue;
		}

		if (!Marker->MarkerId.IsNone())
		{
			if (SeenMarkerIds.Contains(Marker->MarkerId))
			{
				AddValidationIssue(
					Result,
					EScenarioMarkerValidationErrorCode::DuplicateMarkerId,
					Marker->MarkerId,
					FText::Format(
						LOCTEXT("DuplicateMarkerId", "Marker ID '{0}' is duplicated."),
						FText::FromName(Marker->MarkerId)));
			}
			else
			{
				SeenMarkerIds.Add(Marker->MarkerId);
			}

			if (Marker->IsA<AObjectiveAreaActor>())
			{
				ObjectiveIds.Add(Marker->MarkerId);
			}
		}
	}

	for (const AScenarioMarkerActor* Marker : Markers)
	{
		if (IsValid(Marker))
		{
			Marker->AppendValidationIssues(ObjectiveIds, Result);
		}
	}

	return Result;
}

void AScenarioMarkerActor::AppendValidationIssues(
	const TSet<FName>&,
	FScenarioMarkerValidationResult& Result) const
{
	if (MarkerId.IsNone())
	{
		AddValidationIssue(
			Result,
			EScenarioMarkerValidationErrorCode::MissingMarkerId,
			NAME_None,
			LOCTEXT("MissingMarkerId", "Every Scenario marker requires a Marker ID."));
	}
}

void AScenarioMarkerActor::AddValidationIssue(
	FScenarioMarkerValidationResult& Result,
	const EScenarioMarkerValidationErrorCode Code,
	const FName IssueMarkerId,
	const FText& Message)
{
	FScenarioMarkerValidationIssue& Issue =
		Result.Issues.AddDefaulted_GetRef();
	Issue.Code = Code;
	Issue.MarkerId = IssueMarkerId;
	Issue.Message = Message;
}

#undef LOCTEXT_NAMESPACE
