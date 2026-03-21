// Fill out your copyright notice in the Description page of Project Settings.


#include "DummyTarget.h"

#include "PhysicsEngine/PhysicsAsset.h"

// Sets default values
ADummyTarget::ADummyTarget()
{
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ADummyTarget::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnDeath.AddDynamic(this, &ADummyTarget::OnDeath);
}

void ADummyTarget::OnDeath()
{
	// Mesh가 유효한지 먼저 확인
	if (!Mesh)
	{
		UE_LOG(LogTemp, Error, TEXT("[DummyTarget] Mesh is NULL"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[DummyTarget] Mesh: %s"), *Mesh->GetName());
	UE_LOG(LogTemp, Warning, TEXT("[DummyTarget] PhysicsAsset: %s"), 
		Mesh->GetPhysicsAsset() ? *Mesh->GetPhysicsAsset()->GetName() : TEXT("NULL"));

	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
	Mesh->SetAllBodiesSimulatePhysics(true);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetPhysicsBlendWeight(1.f);
}
