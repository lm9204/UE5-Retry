// Fill out your copyright notice in the Description page of Project Settings.


#include "LootWidget.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void ULootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ACharacter* Player = Cast<ACharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player) return;

	OwnerLoot = Player->FindComponentByClass<ULootComponent>();
	if (OwnerLoot)
	{
		OwnerLoot->OnLootRangeChanged.AddDynamic(
			this, &ULootWidget::OnLootRangeChanged);
	}
}

void ULootWidget::RequestLoot(ADroppedItemActor* Item)
{
	if (OwnerLoot) OwnerLoot->LootItem(Item);
}

void ULootWidget::RequestLootAll()
{
	if (OwnerLoot) OwnerLoot->LootAll();
}

void ULootWidget::OnLootRangeChanged(const TArray<ADroppedItemActor*>& NearbyItems)
{
	// 아이템 없으면 위젯 숨기기
	if (NearbyItems.Num() == 0)
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshLootList(NearbyItems);
}
