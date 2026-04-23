#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemFragment.generated.h"

UCLASS(Abstract, DefaultToInstanced, EditInlineNew, BlueprintType)
class RETRY_API UItemFragment : public UObject
{
	GENERATED_BODY()

public:
	// 장착 / 사용 시 호출
	virtual void OnFragmentActivated(AActor* Owner) {}
	virtual void OnFragmentDeactivated(AActor* Owner) {}

	// tooltip 표시용 설명 텍스트
	virtual FText GetFragmentDescription() const
	{
		return FText::GetEmpty();
	}
};