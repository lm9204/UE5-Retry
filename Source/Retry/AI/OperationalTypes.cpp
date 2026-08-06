#include "AI/OperationalTypes.h"

#define LOCTEXT_NAMESPACE "OperationalTypes"

namespace OperationalPredicates
{
	const FName AreaObserved = TEXT("AreaObserved");
	const FName AreaSecured = TEXT("AreaSecured");

	bool BuildAreaReport(
		const FCommandIntent& Command,
		const FMissionContext& Mission,
		const FGuid& RunId,
		const uint8 TeamId,
		const FName SourceGroupId,
		const double EventAtSeconds,
		const ECommandVerb ExpectedVerb,
		const FName SupportedPredicate,
		FOperationalReport& OutReport,
		FText& OutError)
	{
		OutReport = FOperationalReport();
		if (Command.Status != ECommandStatus::Executing
			|| Command.Verb != ExpectedVerb
			|| Command.TargetType != ECommandTargetType::Area
			|| !Command.CommandId.IsValid()
			|| Command.TargetId.IsNone()
			|| Command.CommandId != Mission.CommandId
			|| !RunId.IsValid()
			|| SourceGroupId.IsNone()
			|| Mission.ObjectiveId.IsNone()
			|| Mission.ObjectiveLocation.ContainsNaN()
			|| !FMath::IsFinite(EventAtSeconds)
			|| EventAtSeconds < 0.0)
		{
			OutError = LOCTEXT(
				"InvalidAreaReportInput",
				"An executing Area command, matching Mission, Run, and source are required.");
			return false;
		}

		OutReport.ReportId = FGuid::NewGuid();
		OutReport.RunId = RunId;
		OutReport.CommandId = Command.CommandId;
		OutReport.TeamId = TeamId;
		OutReport.SourceGroupId = SourceGroupId;
		OutReport.Status = EOperationalReportStatus::Created;
		OutReport.CreatedAtSeconds = EventAtSeconds;

		const auto AddFact = [&OutReport, &Mission, EventAtSeconds](
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
			Fact.ObservedAtSeconds = EventAtSeconds;
		};

		for (const FInformationRequirement& Requirement
			: Command.InformationRequirements)
		{
			if (Requirement.RequirementId != SupportedPredicate)
			{
				OutReport = FOperationalReport();
				OutError = FText::Format(
					LOCTEXT(
						"UnsupportedAreaRequirement",
						"This command does not have an evaluator for requirement '{0}'."),
					FText::FromName(Requirement.RequirementId));
				return false;
			}
			AddFact(Requirement.RequirementId, Requirement.SubjectId);
		}

		if (OutReport.Facts.IsEmpty())
		{
			AddFact(SupportedPredicate, Command.TargetId);
		}

		if (!OutReport.IsValid())
		{
			OutReport = FOperationalReport();
			OutError = LOCTEXT(
				"InvalidBuiltAreaReport",
				"The generated operational report is invalid.");
			return false;
		}

		OutError = FText::GetEmpty();
		return true;
	}
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
	return OperationalPredicates::BuildAreaReport(
		Command, Mission, RunId, TeamId, SourceGroupId,
		ObservedAtSeconds, ECommandVerb::Recon,
		OperationalPredicates::AreaObserved, OutReport, OutError);
}

bool BuildSecureAreaOperationalReport(
	const FCommandIntent& Command,
	const FMissionContext& Mission,
	const FGuid& RunId,
	const uint8 TeamId,
	const FName SourceGroupId,
	const double SecuredAtSeconds,
	FOperationalReport& OutReport,
	FText& OutError)
{
	return OperationalPredicates::BuildAreaReport(
		Command, Mission, RunId, TeamId, SourceGroupId,
		SecuredAtSeconds, ECommandVerb::Secure,
		OperationalPredicates::AreaSecured, OutReport, OutError);
}

#undef LOCTEXT_NAMESPACE
