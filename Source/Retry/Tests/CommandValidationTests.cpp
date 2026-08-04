#include "Misc/AutomationTest.h"

#include "AI/CommandTypes.h"
#include "AI/CommandValidation.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace CommandValidationTests
{
	FCommandIntent MakeValidCommand(
		const ECommandVerb Verb,
		const ECommandTargetType TargetType)
	{
		FCommandIntent Command;
		Command.CommandId = FGuid::NewGuid();
		Command.IssuerId = TEXT("HQ");
		Command.AssignedGroupId = TEXT("GroupA");
		Command.Verb = Verb;
		Command.TargetType = TargetType;
		if (TargetType != ECommandTargetType::Position)
		{
			Command.TargetId = TEXT("Target01");
		}
		return Command;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommandStatusAcceptsForwardTransitions,
	"Retry.Command.Status.AcceptsForwardTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandStatusAcceptsForwardTransitions::RunTest(const FString& Parameters)
{
	FCommandIntent Command;
	FText Error;

	TestTrue(TEXT("Proposed에서 Validated로 전이한다."),
		TryTransitionCommandStatus(Command, ECommandStatus::Validated, Error));
	TestTrue(TEXT("Validated에서 Assigned로 전이한다."),
		TryTransitionCommandStatus(Command, ECommandStatus::Assigned, Error));
	TestTrue(TEXT("Assigned에서 Executing으로 전이한다."),
		TryTransitionCommandStatus(Command, ECommandStatus::Executing, Error));
	TestTrue(TEXT("Executing에서 Completed로 전이한다."),
		TryTransitionCommandStatus(Command, ECommandStatus::Completed, Error));
	TestEqual(TEXT("마지막 상태는 Completed다."), Command.Status, ECommandStatus::Completed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommandStatusAllowsCancellationBeforeTerminal,
	"Retry.Command.Status.AllowsCancellationBeforeTerminal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandStatusAllowsCancellationBeforeTerminal::RunTest(const FString& Parameters)
{
	const ECommandStatus NonTerminalStatuses[] =
	{
		ECommandStatus::Proposed,
		ECommandStatus::Validated,
		ECommandStatus::Assigned,
		ECommandStatus::Executing,
	};

	for (const ECommandStatus InitialStatus : NonTerminalStatuses)
	{
		FCommandIntent Command;
		Command.Status = InitialStatus;
		FText Error;
		TestTrue(
			*FString::Printf(TEXT("%s 상태에서 취소할 수 있다."),
				*UEnum::GetValueAsString(InitialStatus)),
			TryTransitionCommandStatus(Command, ECommandStatus::Cancelled, Error));
		TestEqual(TEXT("취소 결과는 Cancelled다."),
			Command.Status, ECommandStatus::Cancelled);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommandStatusRejectsInvalidTransitions,
	"Retry.Command.Status.RejectsInvalidTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandStatusRejectsInvalidTransitions::RunTest(const FString& Parameters)
{
	FCommandIntent Command;
	Command.Status = ECommandStatus::Assigned;
	FText Error;

	TestFalse(TEXT("Assigned에서 Validated로 역전이할 수 없다."),
		TryTransitionCommandStatus(Command, ECommandStatus::Validated, Error));
	TestFalse(TEXT("실행 전 상태를 Failed로 바꿀 수 없다."),
		TryTransitionCommandStatus(Command, ECommandStatus::Failed, Error));
	TestFalse(TEXT("실패 시 오류 설명을 반환한다."), Error.IsEmpty());

	Command.Status = ECommandStatus::Completed;
	TestFalse(TEXT("Completed terminal 상태는 다시 전이할 수 없다."),
		TryTransitionCommandStatus(Command, ECommandStatus::Executing, Error));
	TestEqual(TEXT("거부 후 Completed 상태를 보존한다."),
		Command.Status, ECommandStatus::Completed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommandValidationAcceptsSupportedCombinations,
	"Retry.Command.Validation.AcceptsSupportedCombinations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandValidationAcceptsSupportedCombinations::RunTest(
	const FString& Parameters)
{
	const TPair<ECommandVerb, ECommandTargetType> SupportedCombinations[] =
	{
		{ ECommandVerb::Recon, ECommandTargetType::Area },
		{ ECommandVerb::Recon, ECommandTargetType::Route },
		{ ECommandVerb::Secure, ECommandTargetType::Area },
		{ ECommandVerb::Defend, ECommandTargetType::Position },
		{ ECommandVerb::Block, ECommandTargetType::Route },
	};

	for (const TPair<ECommandVerb, ECommandTargetType>& Combination
		: SupportedCombinations)
	{
		const FCommandValidationResult Result = FCommandValidator::Validate(
			CommandValidationTests::MakeValidCommand(
				Combination.Key, Combination.Value));
		TestTrue(TEXT("지원되는 Verb/Target 조합을 허용한다."), Result.IsValid());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommandValidationRejectsUnsupportedCombinations,
	"Retry.Command.Validation.RejectsUnsupportedCombinations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandValidationRejectsUnsupportedCombinations::RunTest(
	const FString& Parameters)
{
	const TPair<ECommandVerb, ECommandTargetType> UnsupportedCombinations[] =
	{
		{ ECommandVerb::Recon, ECommandTargetType::Position },
		{ ECommandVerb::Secure, ECommandTargetType::Route },
		{ ECommandVerb::Defend, ECommandTargetType::Area },
		{ ECommandVerb::Block, ECommandTargetType::Area },
	};

	for (const TPair<ECommandVerb, ECommandTargetType>& Combination
		: UnsupportedCombinations)
	{
		const FCommandValidationResult Result = FCommandValidator::Validate(
			CommandValidationTests::MakeValidCommand(
				Combination.Key, Combination.Value));
		TestTrue(TEXT("지원하지 않는 조합에 구조화 오류를 반환한다."),
			Result.HasError(ECommandValidationErrorCode::UnsupportedVerbTarget));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommandValidationCollectsIdentityAndRangeErrors,
	"Retry.Command.Validation.CollectsIdentityAndRangeErrors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandValidationCollectsIdentityAndRangeErrors::RunTest(
	const FString& Parameters)
{
	FCommandIntent Command;
	Command.TargetId = NAME_None;
	Command.Priority = 101;

	const FCommandValidationResult Result = FCommandValidator::Validate(Command);
	TestTrue(TEXT("Command ID 오류를 수집한다."),
		Result.HasError(ECommandValidationErrorCode::InvalidCommandId));
	TestTrue(TEXT("Issuer ID 오류를 수집한다."),
		Result.HasError(ECommandValidationErrorCode::MissingIssuerId));
	TestTrue(TEXT("Group ID 오류를 수집한다."),
		Result.HasError(ECommandValidationErrorCode::MissingAssignedGroupId));
	TestTrue(TEXT("Target ID 오류를 수집한다."),
		Result.HasError(ECommandValidationErrorCode::MissingTargetId));
	TestTrue(TEXT("Priority 오류를 수집한다."),
		Result.HasError(ECommandValidationErrorCode::InvalidPriority));
	TestTrue(TEXT("첫 오류에서 멈추지 않는다."), Result.Issues.Num() >= 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommandValidationRejectsInvalidNestedData,
	"Retry.Command.Validation.RejectsInvalidNestedData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandValidationRejectsInvalidNestedData::RunTest(
	const FString& Parameters)
{
	FCommandIntent Command = CommandValidationTests::MakeValidCommand(
		ECommandVerb::Recon, ECommandTargetType::Area);

	FCommandConstraint Constraint;
	Constraint.ConstraintId = TEXT("NoLongRangePursuit");
	Command.Constraints.Add(Constraint);
	Command.Constraints.Add(Constraint);

	FInformationRequirement Requirement;
	Requirement.RequirementId = TEXT("EnemyPresence");
	Command.InformationRequirements.Add(Requirement);

	Command.CompletionCriteria.RequiredConditionIds.Add(TEXT("ReportReceived"));
	Command.CompletionCriteria.RequiredConditionIds.Add(TEXT("ReportReceived"));
	Command.CompletionCriteria.MinimumHoldSeconds = 10.f;
	Command.CompletionCriteria.TimeoutSeconds = 5.f;

	const FCommandValidationResult Result = FCommandValidator::Validate(Command);
	TestTrue(TEXT("중복 Constraint를 거부한다."),
		Result.HasError(ECommandValidationErrorCode::DuplicateConstraintId));
	TestTrue(TEXT("Requirement Subject 누락을 거부한다."),
		Result.HasError(ECommandValidationErrorCode::MissingRequirementSubjectId));
	TestTrue(TEXT("중복 완료 조건을 거부한다."),
		Result.HasError(ECommandValidationErrorCode::DuplicateCompletionConditionId));
	TestTrue(TEXT("Hold가 Timeout보다 긴 모순을 거부한다."),
		Result.HasError(ECommandValidationErrorCode::InvalidCompletionTiming));
	return true;
}

#endif
