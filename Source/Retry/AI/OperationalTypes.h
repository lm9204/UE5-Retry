#pragma once

#include "CoreMinimal.h"
#include "AI/CommandTypes.h"
#include "OperationalTypes.generated.h"

namespace OperationalPredicates
{
	extern const FName AreaObserved;
	extern const FName AreaSecured;
}

UENUM(BlueprintType)
enum class EOperationalReportStatus : uint8
{
	Created,
	Transmitting,
	Received,
};

USTRUCT(BlueprintType)
struct FOperationalFact
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	FGuid FactId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	FGuid CommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	uint8 TeamId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	FName SourceGroupId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	FName PredicateId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	FName SubjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	double ObservedAtSeconds = 0.0;

	bool IsValid() const;
};

USTRUCT(BlueprintType)
struct FOperationalReport
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	FGuid ReportId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	FGuid CommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	uint8 TeamId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	FName SourceGroupId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	EOperationalReportStatus Status = EOperationalReportStatus::Created;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	TArray<FOperationalFact> Facts;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	double CreatedAtSeconds = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operational")
	double ReceivedAtSeconds = 0.0;

	bool IsValid() const;
};

RETRY_API bool BuildReconOperationalReport(
	const FCommandIntent& Command,
	const FMissionContext& Mission,
	const FGuid& RunId,
	uint8 TeamId,
	FName SourceGroupId,
	double ObservedAtSeconds,
	FOperationalReport& OutReport,
	FText& OutError);

RETRY_API bool BuildSecureAreaOperationalReport(
	const FCommandIntent& Command,
	const FMissionContext& Mission,
	const FGuid& RunId,
	uint8 TeamId,
	FName SourceGroupId,
	double SecuredAtSeconds,
	FOperationalReport& OutReport,
	FText& OutError);
