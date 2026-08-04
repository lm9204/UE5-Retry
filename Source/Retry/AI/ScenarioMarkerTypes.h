#pragma once

#include "CoreMinimal.h"
#include "ScenarioMarkerTypes.generated.h"

UENUM(BlueprintType)
enum class EScenarioMarkerValidationErrorCode : uint8
{
	InvalidMarkerReference,
	MissingMarkerId,
	DuplicateMarkerId,
	InvalidAreaRadius,
	MissingObjectiveId,
	UnknownObjectiveId,
};

USTRUCT(BlueprintType)
struct FScenarioMarkerValidationIssue
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Marker")
	EScenarioMarkerValidationErrorCode Code =
		EScenarioMarkerValidationErrorCode::InvalidMarkerReference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Marker")
	FName MarkerId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Marker")
	FText Message;
};

USTRUCT(BlueprintType)
struct FScenarioMarkerValidationResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scenario|Marker")
	TArray<FScenarioMarkerValidationIssue> Issues;

	bool IsValid() const
	{
		return Issues.IsEmpty();
	}

	bool HasError(EScenarioMarkerValidationErrorCode Code) const
	{
		return Issues.ContainsByPredicate(
			[Code](const FScenarioMarkerValidationIssue& Issue)
			{
				return Issue.Code == Code;
			});
	}
};
