#include "AI/ObservationPointActor.h"

#include "Components/ArrowComponent.h"

#define LOCTEXT_NAMESPACE "ObservationPointActor"

AObservationPointActor::AObservationPointActor()
{
	FacingVisualization = CreateDefaultSubobject<UArrowComponent>(
		TEXT("FacingVisualization"));
	FacingVisualization->SetupAttachment(GetRootComponent());
	FacingVisualization->SetHiddenInGame(true);
	FacingVisualization->SetArrowColor(FColor::Cyan);
	FacingVisualization->ArrowSize = 1.5f;
}

FName AObservationPointActor::GetObjectiveId() const
{
	return ObjectiveId;
}

void AObservationPointActor::AppendValidationIssues(
	const TSet<FName>& ObjectiveIds,
	FScenarioMarkerValidationResult& Result) const
{
	Super::AppendValidationIssues(ObjectiveIds, Result);
	if (ObjectiveId.IsNone())
	{
		AddValidationIssue(
			Result,
			EScenarioMarkerValidationErrorCode::MissingObjectiveId,
			MarkerId,
			LOCTEXT("MissingObjectiveId", "Observation Point requires an Objective ID."));
	}
	else if (!ObjectiveIds.Contains(ObjectiveId))
	{
		AddValidationIssue(
			Result,
			EScenarioMarkerValidationErrorCode::UnknownObjectiveId,
			MarkerId,
			FText::Format(
				LOCTEXT("UnknownObjectiveId", "Observation Point references unknown Objective ID '{0}'."),
				FText::FromName(ObjectiveId)));
	}
}

#undef LOCTEXT_NAMESPACE
