#include "AI/CommandValidation.h"

#define LOCTEXT_NAMESPACE "CommandValidation"

namespace CommandValidation
{
	void AddIssue(
		FCommandValidationResult& Result,
		const ECommandValidationErrorCode Code,
		const FText& Message)
	{
		FCommandValidationIssue& Issue = Result.Issues.AddDefaulted_GetRef();
		Issue.Code = Code;
		Issue.Message = Message;
	}

	bool IsSupportedVerbTarget(
		const ECommandVerb Verb, const ECommandTargetType TargetType)
	{
		switch (Verb)
		{
		case ECommandVerb::Recon:
			return TargetType == ECommandTargetType::Area
				|| TargetType == ECommandTargetType::Route;
		case ECommandVerb::Secure:
			return TargetType == ECommandTargetType::Area;
		case ECommandVerb::Defend:
			return TargetType == ECommandTargetType::Position;
		case ECommandVerb::Block:
			return TargetType == ECommandTargetType::Route;
		default:
			return false;
		}
	}

	bool RequiresTargetId(const ECommandTargetType TargetType)
	{
		return TargetType != ECommandTargetType::Position;
	}
}

bool FCommandValidationResult::HasError(
	const ECommandValidationErrorCode Code) const
{
	return Issues.ContainsByPredicate(
		[Code](const FCommandValidationIssue& Issue)
		{
			return Issue.Code == Code;
		});
}

FCommandValidationResult FCommandValidator::Validate(
	const FCommandIntent& Command)
{
	FCommandValidationResult Result;

	if (Command.Status != ECommandStatus::Proposed)
	{
		CommandValidation::AddIssue(
			Result, ECommandValidationErrorCode::InvalidInitialStatus,
			LOCTEXT("InvalidInitialStatus", "Only a Proposed command can be validated."));
	}

	if (!Command.CommandId.IsValid())
	{
		CommandValidation::AddIssue(
			Result, ECommandValidationErrorCode::InvalidCommandId,
			LOCTEXT("InvalidCommandId", "Command ID must be a valid GUID."));
	}

	if (Command.IssuerId.IsNone())
	{
		CommandValidation::AddIssue(
			Result, ECommandValidationErrorCode::MissingIssuerId,
			LOCTEXT("MissingIssuerId", "Issuer ID is required."));
	}

	if (Command.AssignedGroupId.IsNone())
	{
		CommandValidation::AddIssue(
			Result, ECommandValidationErrorCode::MissingAssignedGroupId,
			LOCTEXT("MissingAssignedGroupId", "Assigned Group ID is required."));
	}

	if (!CommandValidation::IsSupportedVerbTarget(
		Command.Verb, Command.TargetType))
	{
		CommandValidation::AddIssue(
			Result, ECommandValidationErrorCode::UnsupportedVerbTarget,
			LOCTEXT("UnsupportedVerbTarget", "The command verb and target type combination is not supported."));
	}

	if (CommandValidation::RequiresTargetId(Command.TargetType)
		&& Command.TargetId.IsNone())
	{
		CommandValidation::AddIssue(
			Result, ECommandValidationErrorCode::MissingTargetId,
			LOCTEXT("MissingTargetId", "This target type requires a Target ID."));
	}

	if (Command.TargetType == ECommandTargetType::Position
		&& Command.TargetLocation.ContainsNaN())
	{
		CommandValidation::AddIssue(
			Result, ECommandValidationErrorCode::InvalidTargetLocation,
			LOCTEXT("InvalidTargetLocation", "Target Location must contain finite values."));
	}

	if (Command.Priority < 0 || Command.Priority > 100)
	{
		CommandValidation::AddIssue(
			Result, ECommandValidationErrorCode::InvalidPriority,
			LOCTEXT("InvalidPriority", "Priority must be between 0 and 100."));
	}

	TSet<FName> ConstraintIds;
	for (const FCommandConstraint& Constraint : Command.Constraints)
	{
		if (Constraint.ConstraintId.IsNone())
		{
			CommandValidation::AddIssue(
				Result, ECommandValidationErrorCode::MissingConstraintId,
				LOCTEXT("MissingConstraintId", "Every constraint requires an ID."));
		}
		else if (ConstraintIds.Contains(Constraint.ConstraintId))
		{
			CommandValidation::AddIssue(
				Result, ECommandValidationErrorCode::DuplicateConstraintId,
				FText::Format(
					LOCTEXT("DuplicateConstraintId", "Constraint ID '{0}' is duplicated."),
					FText::FromName(Constraint.ConstraintId)));
		}
		else
		{
			ConstraintIds.Add(Constraint.ConstraintId);
		}

		if (!FMath::IsFinite(Constraint.NumericValue))
		{
			CommandValidation::AddIssue(
				Result, ECommandValidationErrorCode::InvalidConstraintValue,
				LOCTEXT("InvalidConstraintValue", "Constraint numeric values must be finite."));
		}
	}

	TSet<FString> RequirementKeys;
	for (const FInformationRequirement& Requirement
		: Command.InformationRequirements)
	{
		if (Requirement.RequirementId.IsNone())
		{
			CommandValidation::AddIssue(
				Result, ECommandValidationErrorCode::MissingRequirementId,
				LOCTEXT("MissingRequirementId", "Every information requirement requires an ID."));
		}

		if (Requirement.SubjectId.IsNone())
		{
			CommandValidation::AddIssue(
				Result, ECommandValidationErrorCode::MissingRequirementSubjectId,
				LOCTEXT("MissingRequirementSubjectId", "Every information requirement requires a Subject ID."));
		}

		if (!Requirement.RequirementId.IsNone()
			&& !Requirement.SubjectId.IsNone())
		{
			const FString RequirementKey = FString::Printf(
				TEXT("%s|%s"),
				*Requirement.RequirementId.ToString(),
				*Requirement.SubjectId.ToString());
			if (RequirementKeys.Contains(RequirementKey))
			{
				CommandValidation::AddIssue(
					Result, ECommandValidationErrorCode::DuplicateInformationRequirement,
					LOCTEXT("DuplicateInformationRequirement", "An information requirement is duplicated."));
			}
			else
			{
				RequirementKeys.Add(RequirementKey);
			}
		}
	}

	TSet<FName> CompletionConditionIds;
	for (const FName ConditionId
		: Command.CompletionCriteria.RequiredConditionIds)
	{
		if (ConditionId.IsNone())
		{
			CommandValidation::AddIssue(
				Result, ECommandValidationErrorCode::InvalidCompletionConditionId,
				LOCTEXT("InvalidCompletionConditionId", "Completion condition IDs cannot be empty."));
		}
		else if (CompletionConditionIds.Contains(ConditionId))
		{
			CommandValidation::AddIssue(
				Result, ECommandValidationErrorCode::DuplicateCompletionConditionId,
				FText::Format(
					LOCTEXT("DuplicateCompletionConditionId", "Completion condition ID '{0}' is duplicated."),
					FText::FromName(ConditionId)));
		}
		else
		{
			CompletionConditionIds.Add(ConditionId);
		}
	}

	const float MinimumHoldSeconds =
		Command.CompletionCriteria.MinimumHoldSeconds;
	const float TimeoutSeconds = Command.CompletionCriteria.TimeoutSeconds;
	if (!FMath::IsFinite(MinimumHoldSeconds)
		|| !FMath::IsFinite(TimeoutSeconds)
		|| MinimumHoldSeconds < 0.f
		|| TimeoutSeconds < 0.f
		|| (TimeoutSeconds > 0.f && MinimumHoldSeconds > TimeoutSeconds))
	{
		CommandValidation::AddIssue(
			Result, ECommandValidationErrorCode::InvalidCompletionTiming,
			LOCTEXT("InvalidCompletionTiming", "Completion hold and timeout values are inconsistent."));
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE
