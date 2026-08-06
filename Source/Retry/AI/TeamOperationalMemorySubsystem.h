#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AI/OperationalTypes.h"
#include "TeamOperationalMemorySubsystem.generated.h"

UCLASS()
class RETRY_API UTeamOperationalMemorySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	bool ReceiveReport(
		const FOperationalReport& Report,
		FOperationalReport& OutReceivedReport,
		FText& OutError);

	bool HasReceivedRequirement(
		uint8 TeamId,
		const FGuid& RunId,
		const FGuid& CommandId,
		const FInformationRequirement& Requirement) const;

	bool HasReceivedFact(
		uint8 TeamId,
		const FGuid& RunId,
		FName PredicateId,
		FName SubjectId,
		FName SourceGroupId = NAME_None) const;

	TArray<FOperationalFact> GetFactsForTeam(uint8 TeamId) const;
	TArray<FOperationalReport> GetReceivedReportsForTeam(uint8 TeamId) const;
	void ResetOperationalMemory();

private:
	TMap<uint8, TArray<FOperationalFact>> FactsByTeam;
	TMap<FGuid, FOperationalReport> ReportsById;
};
