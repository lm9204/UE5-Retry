// Fill out your copyright notice in the Description page of Project Settings.


#include "DroppedItemActor.h"

#include "Components/LootComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"

// Sets default values
ADroppedItemActor::ADroppedItemActor()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));

	VisualSphere = CreateDefaultSubobject<USphereComponent>(TEXT("VisualSphere"));
	VisualSphere->SetupAttachment(RootComponent);
	VisualSphere->InitSphereRadius(10.f);		
	VisualSphere->SetHiddenInGame(false);

	LootTrigger = CreateDefaultSubobject<USphereComponent>("LootTrigger");
	LootTrigger->SetupAttachment(RootComponent);
	LootTrigger->SetSphereRadius(LootRange);
	LootTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ADroppedItemActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}


// Called when the game starts or when spawned
void ADroppedItemActor::BeginPlay()
{
	Super::BeginPlay();

	LootTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADroppedItemActor::OnLootRangeBeginOverlap);
	LootTrigger->OnComponentEndOverlap.AddDynamic(this, &ADroppedItemActor::OnLootRangeEndOverlap);
}

void ADroppedItemActor::OnLootRangeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 플레이어가 범위 안에 들어오면 루팅 UI 표시
	// LootComponent에서 처리
	ACharacter* Player = Cast<ACharacter>(OtherActor);
	if (!Player) return;

	ULootComponent* LootComp = Player->FindComponentByClass<ULootComponent>();
	if (LootComp) LootComp->AddNearbyItem(this);
}

void ADroppedItemActor::OnLootRangeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ACharacter* Player = Cast<ACharacter>(OtherActor);
	if (!Player) return;

	ULootComponent* LootComp = Player->FindComponentByClass<ULootComponent>();
	if (LootComp) LootComp->RemoveNearbyItem(this);
}
