#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_Decision.generated.h"

UCLASS()
class RETRY_API UBTService_Decision : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_Decision();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory, float DeltaSeconds) override;
};