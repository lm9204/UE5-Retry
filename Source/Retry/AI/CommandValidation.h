#pragma once

#include "CoreMinimal.h"
#include "AI/CommandTypes.h"
#include "CommandValidation.generated.h"

UENUM(BlueprintType)
enum class ECommandValidationErrorCode : uint8
{
	InvalidInitialStatus,
	InvalidCommandId,
	MissingIssuerId,
	MissingAssignedGroupId,
	UnsupportedVerbTarget,
	MissingTargetId,
	InvalidTargetLocation,
	InvalidPriority,
	MissingConstraintId,
	DuplicateConstraintId,
	InvalidConstraintValue,
	MissingRequirementId,
	MissingRequirementSubjectId,
	DuplicateInformationRequirement,
	InvalidCompletionConditionId,
	DuplicateCompletionConditionId,
	InvalidCompletionTiming,
};

USTRUCT(BlueprintType)
struct FCommandValidationIssue
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Command|Validation")
	ECommandValidationErrorCode Code =
		ECommandValidationErrorCode::InvalidCommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Command|Validation")
	FText Message;
};

USTRUCT(BlueprintType)
struct FCommandValidationResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Command|Validation")
	TArray<FCommandValidationIssue> Issues;

	bool IsValid() const
	{
		return Issues.IsEmpty();
	}

	bool HasError(ECommandValidationErrorCode Code) const;
};

class RETRY_API FCommandValidator
{
public:
	static FCommandValidationResult Validate(const FCommandIntent& Command);
};
