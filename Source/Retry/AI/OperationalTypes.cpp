#include "AI/OperationalTypes.h"

#define LOCTEXT_NAMESPACE "OperationalTypes"

namespace OperationalPredicates
{
	const FName AreaObserved = TEXT("AreaObserved");
}

bool FOperationalFact::IsValid() const
{
	return FactId.IsValid()
		&& RunId.IsValid()
		&& CommandId.IsValid()
		&& !SourceGroupId.IsNone()
		&& !PredicateId.IsNone()
		&& !SubjectId.IsNone()
		&& !Location.ContainsNaN()
		&& FMath::IsFinite(ObservedAtSeconds)
		&& ObservedAtSeconds >= 0.0;
}

bool FOperationalReport::IsValid() const
{
	if (!ReportId.IsValid()
		|| !RunId.IsValid()
		|| !CommandId.IsValid()
		|| SourceGroupId.IsNone()
		|| Facts.IsEmpty()
		|| !FMath::IsFinite(CreatedAtSeconds)
		|| CreatedAtSeconds < 0.0)
	{
		return false;
	}

	for (const FOperationalFact& Fact : Facts)
	{
		if (!Fact.IsValid()
			|| Fact.RunId != RunId
			|| Fact.CommandId != CommandId
			|| Fact.TeamId != TeamId
			|| Fact.SourceGroupId != SourceGroupId)
		{
			return false;
		}
	}

	return Status != EOperationalReportStatus::Received
		|| (FMath::IsFinite(ReceivedAtSeconds)
			&& ReceivedAtSeconds >= CreatedAtSeconds);
}

bool BuildReconOperationalReport(
	const FCommandIntent& Command,
	const FMissionContext& Mission,
	const FGuid& RunId,
	const uint8 TeamId,
	const FName SourceGroupId,
	const double ObservedAtSeconds,
	FOperationalReport& OutReport,
	FText& OutError)
{
	OutReport = FOperationalReport();
	if (Command.Status != ECommandStatus::Executing
		|| Command.Verb != ECommandVerb::Recon
		|| Command.TargetType != ECommandTargetType::Area
		|| !Command.CommandId.IsValid()
		|| Command.TargetId.IsNone()
		|| Command.CommandId != Mission.CommandId
		|| !RunId.IsValid()
		|| SourceGroupId.IsNone()
		|| Mission.ObjectiveId.IsNone()
		|| Mission.ObjectiveLocation.ContainsNaN()
		|| !FMath::IsFinite(ObservedAtSeconds)
		|| ObservedAtSeconds < 0.0)
	{
		OutError = LOCTEXT(
			"InvalidReconReportInput",
			"An executing Recon command, matching Mission, Run, and source are required.");
		return false;
	}

	OutReport.ReportId = FGuid::NewGuid();
	OutReport.RunId = RunId;
	OutReport.CommandId = Command.CommandId;
	OutReport.TeamId = TeamId;
	OutReport.SourceGroupId = SourceGroupId;
	OutReport.Status = EOperationalReportStatus::Created;
	OutReport.CreatedAtSeconds = ObservedAtSeconds;

	const auto AddFact = [&OutReport, &Mission, ObservedAtSeconds](
		const FName PredicateId,
		const FName SubjectId)
	{
		FOperationalFact& Fact = OutReport.Facts.AddDefaulted_GetRef();
		Fact.FactId = FGuid::NewGuid();
		Fact.RunId = OutReport.RunId;
		Fact.CommandId = OutReport.CommandId;
		Fact.TeamId = OutReport.TeamId;
		Fact.SourceGroupId = OutReport.SourceGroupId;
		Fact.PredicateId = PredicateId;
		Fact.SubjectId = SubjectId;
		Fact.Location = Mission.ObjectiveLocation;
		Fact.ObservedAtSeconds = ObservedAtSeconds;
	};

	for (const FInformationRequirement& Requirement
		: Command.InformationRequirements)
	{
		if (Requirement.RequirementId != OperationalPredicates::AreaObserved)
		{
			OutReport = FOperationalReport();
			OutError = FText::Format(
				LOCTEXT(
					"UnsupportedReconRequirement",
					"Recon does not yet have an evaluator for requirement '{0}'."),
				FText::FromName(Requirement.RequirementId));
			return false;
		}
		AddFact(Requirement.RequirementId, Requirement.SubjectId);
	}

	if (OutReport.Facts.IsEmpty())
	{
		AddFact(OperationalPredicates::AreaObserved, Command.TargetId);
	}

	if (!OutReport.IsValid())
	{
		OutReport = FOperationalReport();
		OutError = LOCTEXT(
			"InvalidBuiltReconReport",
			"The generated Recon report is invalid.");
		return false;
	}

	OutError = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
