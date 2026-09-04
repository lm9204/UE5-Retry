#include "AI/OperationalObjectiveTypes.h"

#define LOCTEXT_NAMESPACE "OperationalObjectiveTypes"

namespace OperationalObjectiveStates
{
	const FName MaintainAreaControl = TEXT("MaintainAreaControl");
}

bool FOperationalObjective::IsValid() const
{
	return ObjectiveInstanceId.IsValid()
		&& RunId.IsValid()
		&& SourceFactId.IsValid()
		&& !ObjectiveId.IsNone()
		&& DesiredStateId == OperationalObjectiveStates::MaintainAreaControl
		&& !SubjectId.IsNone()
		&& !TargetLocation.ContainsNaN()
		&& Priority >= 0
		&& Priority <= 100;
}

bool BuildMaintainAreaControlObjective(
	const FGuid ObjectiveInstanceId,
	const FName ObjectiveId,
	const int32 Priority,
	const FOperationalFact& AreaSecuredFact,
	FOperationalObjective& OutObjective,
	FText& OutError)
{
	OutObjective = FOperationalObjective();
	if (!ObjectiveInstanceId.IsValid()
		|| ObjectiveId.IsNone()
		|| Priority < 0
		|| Priority > 100
		|| !AreaSecuredFact.IsValid()
		|| AreaSecuredFact.PredicateId != OperationalPredicates::AreaSecured)
	{
		OutError = LOCTEXT(
			"InvalidMaintainControlSource",
			"Maintain Area Control requires a valid AreaSecured Fact and objective identity.");
		return false;
	}

	OutObjective.ObjectiveInstanceId = ObjectiveInstanceId;
	OutObjective.RunId = AreaSecuredFact.RunId;
	OutObjective.SourceFactId = AreaSecuredFact.FactId;
	OutObjective.ObjectiveId = ObjectiveId;
	OutObjective.DesiredStateId =
		OperationalObjectiveStates::MaintainAreaControl;
	OutObjective.SubjectId = AreaSecuredFact.SubjectId;
	OutObjective.TargetLocation = AreaSecuredFact.Location;
	OutObjective.TeamId = AreaSecuredFact.TeamId;
	OutObjective.Priority = Priority;

	if (!OutObjective.IsValid())
	{
		OutObjective = FOperationalObjective();
		OutError = LOCTEXT(
			"InvalidMaintainControlObjective",
			"The AreaSecured Fact could not produce a valid operational objective.");
		return false;
	}

	OutError = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
