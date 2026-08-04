#include "AI/ObjectiveAreaActor.h"

#include "Components/SphereComponent.h"

#define LOCTEXT_NAMESPACE "ObjectiveAreaActor"

AObjectiveAreaActor::AObjectiveAreaActor()
{
	AreaVisualization = CreateDefaultSubobject<USphereComponent>(
		TEXT("AreaVisualization"));
	AreaVisualization->SetupAttachment(GetRootComponent());
	AreaVisualization->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AreaVisualization->SetGenerateOverlapEvents(false);
	AreaVisualization->SetCanEverAffectNavigation(false);
	AreaVisualization->SetHiddenInGame(true);
	AreaVisualization->SetSphereRadius(AreaRadius);
}

void AObjectiveAreaActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	AreaVisualization->SetSphereRadius(FMath::Max(1.f, AreaRadius));
}

float AObjectiveAreaActor::GetAreaRadius() const
{
	return AreaRadius;
}

void AObjectiveAreaActor::AppendValidationIssues(
	const TSet<FName>& ObjectiveIds,
	FScenarioMarkerValidationResult& Result) const
{
	Super::AppendValidationIssues(ObjectiveIds, Result);
	if (!FMath::IsFinite(AreaRadius) || AreaRadius <= 0.f)
	{
		AddValidationIssue(
			Result,
			EScenarioMarkerValidationErrorCode::InvalidAreaRadius,
			MarkerId,
			LOCTEXT("InvalidAreaRadius", "Objective Area radius must be finite and greater than zero."));
	}
}

#undef LOCTEXT_NAMESPACE
