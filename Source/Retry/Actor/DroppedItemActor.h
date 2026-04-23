// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/ItemTypes.h"
#include "Components/SphereComponent.h"
#include "DroppedItemActor.generated.h"

UCLASS()
class RETRY_API ADroppedItemActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADroppedItemActor();

	// 드롭된 아이템 데이터
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	FItemInstance ItemData;

	// 루팅 가능 범위
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	float LootRange = 150.f;

	UFUNCTION(BlueprintCallable, Category = "Item")
	FItemInstance GetItemData() const { return ItemData; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* VisualSphere;

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* LootTrigger;

	UFUNCTION()
	void OnLootRangeBeginOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnLootRangeEndOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
	
};
