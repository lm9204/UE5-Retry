#pragma once

#include "CoreMinimal.h"
#include "CommandTypes.generated.h"

UENUM(BlueprintType)
enum class ECommandVerb : uint8
{
	Recon,
	Secure,
	Defend,
	Block,
};

UENUM(BlueprintType)
enum class ECommandTargetType : uint8
{
	Area,
	Route,
	Position,
	Unit,
	Information,
};

UENUM(BlueprintType)
enum class ECommandStatus : uint8
{
	Proposed,
	Validated,
	Assigned,
	Executing,
	Completed,
	Failed,
	Cancelled,
};

/** A semantic restriction that a Mission Resolver interprets for a group. */
USTRUCT(BlueprintType)
struct FCommandConstraint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	FName ConstraintId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	bool bIsHardConstraint = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	FName ReferenceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	float NumericValue = 0.f;
};

/** A fact that must be observed and reported for command completion. */
USTRUCT(BlueprintType)
struct FInformationRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	FName RequirementId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	FName SubjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	bool bRequired = true;
};

/** Command-level completion and timeout inputs, evaluated by a monitor later. */
USTRUCT(BlueprintType)
struct FCommandCompletionCriteria
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	TArray<FName> RequiredConditionIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command", meta=(ClampMin="0.0"))
	float MinimumHoldSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command", meta=(ClampMin="0.0"))
	float TimeoutSeconds = 0.f;
};

/** An operational goal before it is resolved into concrete NPC execution data. */
USTRUCT(BlueprintType)
struct FCommandIntent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Command")
	FGuid CommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Command")
	FGuid ParentCommandId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	FName IssuerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	FName AssignedGroupId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	ECommandVerb Verb = ECommandVerb::Recon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	ECommandTargetType TargetType = ECommandTargetType::Area;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	FName TargetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command", meta=(ClampMin="0", ClampMax="100"))
	int32 Priority = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	TArray<FCommandConstraint> Constraints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	TArray<FInformationRequirement> InformationRequirements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
	FCommandCompletionCriteria CompletionCriteria;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Command")
	ECommandStatus Status = ECommandStatus::Proposed;
};

/** Concrete mission data derived from a validated command in a later phase. */
USTRUCT(BlueprintType)
struct FMissionContext
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
	FGuid CommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
	FName ObjectiveId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
	FVector ObjectiveLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
	TArray<FCommandConstraint> HardConstraints;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
	TArray<FCommandConstraint> SoftConstraints;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
	TMap<FName, float> DecisionWeightModifiers;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
	TArray<FInformationRequirement> InformationRequirements;
};

RETRY_API bool IsCommandStatusTerminal(ECommandStatus Status);
RETRY_API bool CanTransitionCommandStatus(ECommandStatus From, ECommandStatus To);
RETRY_API bool TryTransitionCommandStatus(
	FCommandIntent& Command,
	ECommandStatus NewStatus,
	FText& OutError);
