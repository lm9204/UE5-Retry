#include "AI/TeamOperationalMemorySubsystem.h"

#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "TeamOperationalMemorySubsystem"

bool UTeamOperationalMemorySubsystem::ReceiveReport(
	const FOperationalReport& Report,
	FOperationalReport& OutReceivedReport,
	FText& OutError)
{
	OutReceivedReport = FOperationalReport();
	if (!Report.IsValid()
		|| Report.Status == EOperationalReportStatus::Received)
	{
		OutError = LOCTEXT(
			"InvalidIncomingReport",
			"Only a valid Created or Transmitting report can be received.");
		return false;
	}

	if (const FOperationalReport* Existing = ReportsById.Find(Report.ReportId))
	{
		if (Existing->RunId != Report.RunId
			|| Existing->CommandId != Report.CommandId
			|| Existing->TeamId != Report.TeamId
			|| Existing->SourceGroupId != Report.SourceGroupId
			|| Existing->Facts.Num() != Report.Facts.Num())
		{
			OutError = LOCTEXT(
				"ConflictingReportIdentity",
				"A Report ID cannot be reused for different operational data.");
			return false;
		}
		for (int32 Index = 0; Index < Existing->Facts.Num(); ++Index)
		{
			if (Existing->Facts[Index].FactId != Report.Facts[Index].FactId)
			{
				OutError = LOCTEXT(
					"ConflictingReportFacts",
					"A Report ID cannot be reused for different Facts.");
				return false;
			}
		}
		OutReceivedReport = *Existing;
		OutError = FText::GetEmpty();
		return true;
	}

	OutReceivedReport = Report;
	OutReceivedReport.Status = EOperationalReportStatus::Received;
	const UWorld* World = GetWorld();
	OutReceivedReport.ReceivedAtSeconds = World
		? FMath::Max(
			OutReceivedReport.CreatedAtSeconds,
			static_cast<double>(World->GetTimeSeconds()))
		: OutReceivedReport.CreatedAtSeconds;

	if (!OutReceivedReport.IsValid())
	{
		OutReceivedReport = FOperationalReport();
		OutError = LOCTEXT(
			"InvalidReceivedReport",
			"The received report could not be normalized.");
		return false;
	}

	ReportsById.Add(OutReceivedReport.ReportId, OutReceivedReport);
	TArray<FOperationalFact>& TeamFacts =
		FactsByTeam.FindOrAdd(OutReceivedReport.TeamId);
	for (const FOperationalFact& Fact : OutReceivedReport.Facts)
	{
		TeamFacts.Add(Fact);
	}

	OutError = FText::GetEmpty();
	return true;
}

bool UTeamOperationalMemorySubsystem::HasReceivedRequirement(
	const uint8 TeamId,
	const FGuid& RunId,
	const FGuid& CommandId,
	const FInformationRequirement& Requirement) const
{
	const TArray<FOperationalFact>* TeamFacts = FactsByTeam.Find(TeamId);
	if (!TeamFacts || !RunId.IsValid() || !CommandId.IsValid()
		|| Requirement.RequirementId.IsNone()
		|| Requirement.SubjectId.IsNone())
	{
		return false;
	}

	return TeamFacts->ContainsByPredicate(
		[&RunId, &CommandId, &Requirement](const FOperationalFact& Fact)
		{
			return Fact.RunId == RunId
				&& Fact.CommandId == CommandId
				&& Fact.PredicateId == Requirement.RequirementId
				&& Fact.SubjectId == Requirement.SubjectId;
		});
}

TArray<FOperationalFact>
UTeamOperationalMemorySubsystem::GetFactsForTeam(const uint8 TeamId) const
{
	if (const TArray<FOperationalFact>* Facts = FactsByTeam.Find(TeamId))
	{
		return *Facts;
	}
	return {};
}

TArray<FOperationalReport>
UTeamOperationalMemorySubsystem::GetReceivedReportsForTeam(
	const uint8 TeamId) const
{
	TArray<FOperationalReport> Result;
	for (const TPair<FGuid, FOperationalReport>& Pair : ReportsById)
	{
		if (Pair.Value.TeamId == TeamId)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

void UTeamOperationalMemorySubsystem::ResetOperationalMemory()
{
	FactsByTeam.Reset();
	ReportsById.Reset();
}

#undef LOCTEXT_NAMESPACE
