
#include "BT/BTService_Decision.h"
#include "AIController.h"
#include "Components/NPCDecisionComponent.h"

UBTService_Decision::UBTService_Decision()
{
	NodeName = TEXT("Decision");
	Interval = 0.1f;
	RandomDeviation = 0.02f;
}

void UBTService_Decision::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return;

	if (UNPCDecisionComponent* DC =
		AIC->FindComponentByClass<UNPCDecisionComponent>())
	{
		DC->Update(DeltaSeconds);
	}
}