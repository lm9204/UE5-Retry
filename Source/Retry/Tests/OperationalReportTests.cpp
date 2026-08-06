#include "Misc/AutomationTest.h"

#include "AI/OperationalTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace OperationalReportTests
{
	FCommandIntent MakeCommand()
	{
		FCommandIntent Command;
		Command.CommandId = FGuid::NewGuid();
		Command.IssuerId = TEXT("HQ");
		Command.AssignedGroupId = TEXT("A");
		Command.Verb = ECommandVerb::Recon;
		Command.TargetType = ECommandTargetType::Area;
		Command.TargetId = TEXT("ReconArea_A");
		Command.Status = ECommandStatus::Executing;
		return Command;
	}

	FMissionContext MakeMission(const FGuid& CommandId)
	{
		FMissionContext Mission;
		Mission.CommandId = CommandId;
		Mission.ObjectiveId = TEXT("ReconObs_A1");
		Mission.ObjectiveLocation = FVector(100.0, 200.0, 10.0);
		return Mission;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOperationalReportBuildsSecureAreaFact,
	"Retry.Operational.Report.BuildsSecureAreaFact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOperationalReportBuildsSecureAreaFact::RunTest(
	const FString& Parameters)
{
	FCommandIntent Command = OperationalReportTests::MakeCommand();
	Command.Verb = ECommandVerb::Secure;
	FInformationRequirement& Requirement =
		Command.InformationRequirements.AddDefaulted_GetRef();
	Requirement.RequirementId = TEXT("AreaSecured");
	Requirement.SubjectId = TEXT("ReconArea_A");
	FMissionContext Mission =
		OperationalReportTests::MakeMission(Command.CommandId);
	Mission.ObjectiveId = TEXT("ReconArea_A");
	FOperationalReport Report;
	FText Error;

	TestTrue(TEXT("Secure completion builds an operational report."),
		BuildSecureAreaOperationalReport(
			Command, Mission, FGuid::NewGuid(), 1, TEXT("A"),
			10.0, Report, Error));
	TestEqual(TEXT("One AreaSecured Fact is emitted."),
		Report.Facts.Num(), 1);
	if (Report.Facts.Num() == 1)
	{
		TestEqual(TEXT("The secured predicate is explicit."),
			Report.Facts[0].PredicateId, FName(TEXT("AreaSecured")));
		TestEqual(TEXT("The secured Area is the subject."),
			Report.Facts[0].SubjectId, FName(TEXT("ReconArea_A")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOperationalReportBuildsRequiredFacts,
	"Retry.Operational.Report.BuildsRequiredFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOperationalReportBuildsRequiredFacts::RunTest(
	const FString& Parameters)
{
	FCommandIntent Command = OperationalReportTests::MakeCommand();
	FInformationRequirement& Requirement =
		Command.InformationRequirements.AddDefaulted_GetRef();
	Requirement.RequirementId = TEXT("AreaObserved");
	Requirement.SubjectId = TEXT("ReconArea_A");

	FOperationalReport Report;
	FText Error;
	const FGuid RunId = FGuid::NewGuid();
	TestTrue(TEXT("Valid Recon observation builds a report."),
		BuildReconOperationalReport(
			Command,
			OperationalReportTests::MakeMission(Command.CommandId),
			RunId,
			1,
			TEXT("A"),
			12.0,
			Report,
			Error));
	TestTrue(TEXT("Report identity is generated."), Report.ReportId.IsValid());
	TestEqual(TEXT("One requirement produces one Fact."),
		Report.Facts.Num(), 1);
	if (Report.Facts.Num() != 1)
	{
		return false;
	}
	TestEqual(TEXT("Requirement predicate is preserved."),
		Report.Facts[0].PredicateId, Requirement.RequirementId);
	TestEqual(TEXT("Command identity links Fact and Report."),
		Report.Facts[0].CommandId, Report.CommandId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOperationalReportProvidesImplicitReconFact,
	"Retry.Operational.Report.ProvidesImplicitReconFact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOperationalReportProvidesImplicitReconFact::RunTest(
	const FString& Parameters)
{
	const FCommandIntent Command = OperationalReportTests::MakeCommand();
	FOperationalReport Report;
	FText Error;
	TestTrue(TEXT("Recon without explicit requirements still reports arrival."),
		BuildReconOperationalReport(
			Command,
			OperationalReportTests::MakeMission(Command.CommandId),
			FGuid::NewGuid(),
			1,
			TEXT("A"),
			1.0,
			Report,
			Error));
	if (Report.Facts.Num() != 1)
	{
		return false;
	}
	TestEqual(TEXT("Implicit predicate is AreaObserved."),
		Report.Facts[0].PredicateId, FName(TEXT("AreaObserved")));
	TestEqual(TEXT("Implicit subject is the commanded Area."),
		Report.Facts[0].SubjectId, Command.TargetId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOperationalReportRejectsMismatchedMission,
	"Retry.Operational.Report.RejectsMismatchedMission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOperationalReportRejectsMismatchedMission::RunTest(
	const FString& Parameters)
{
	const FCommandIntent Command = OperationalReportTests::MakeCommand();
	FMissionContext Mission =
		OperationalReportTests::MakeMission(FGuid::NewGuid());
	FOperationalReport Report;
	FText Error;
	TestFalse(TEXT("A Mission from another Command is rejected."),
		BuildReconOperationalReport(
			Command,
			Mission,
			FGuid::NewGuid(),
			1,
			TEXT("A"),
			1.0,
			Report,
			Error));
	TestFalse(TEXT("Rejected report has no identity."),
		Report.ReportId.IsValid());

	FCommandIntent UnsupportedCommand = Command;
	FInformationRequirement& Unsupported =
		UnsupportedCommand.InformationRequirements.AddDefaulted_GetRef();
	Unsupported.RequirementId = TEXT("EnemyCountKnown");
	Unsupported.SubjectId = TEXT("ReconArea_A");
	Mission.CommandId = UnsupportedCommand.CommandId;
	TestFalse(TEXT("Unsupported predicates are not invented as observed Facts."),
		BuildReconOperationalReport(
			UnsupportedCommand,
			Mission,
			FGuid::NewGuid(),
			1,
			TEXT("A"),
			1.0,
			Report,
			Error));
	return true;
}

#endif
