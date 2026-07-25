// Copyright Opus 5 Three Games. Original work.

#include "SFResourceNode.h"
#include "SFBuildComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

ASFResourceNode::ASFResourceNode()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECC_WorldStatic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CubeFinder.Object);
	}

	SetCanBeDamaged(true);
}

void ASFResourceNode::InitNode(ESFBuildMaterial InMaterial)
{
	Material = InMaterial;

	// Rough silhouettes so the three node types read differently at a distance:
	// tall and thin for wood, squat for stone, boxy for metal.
	switch (Material)
	{
		case ESFBuildMaterial::Wood:
			Mesh->SetWorldScale3D(FVector(0.8f, 0.8f, 4.5f));
			NodeHealth = 180.f;
			break;
		case ESFBuildMaterial::Stone:
			Mesh->SetWorldScale3D(FVector(2.2f, 2.2f, 1.6f));
			NodeHealth = 260.f;
			break;
		case ESFBuildMaterial::Metal:
			Mesh->SetWorldScale3D(FVector(2.6f, 1.6f, 1.6f));
			NodeHealth = 340.f;
			break;
	}
}

void ASFResourceNode::GrantMaterials(AActor* ToActor, int32 Amount)
{
	if (!ToActor || Amount <= 0)
	{
		return;
	}
	if (USFBuildComponent* Build = ToActor->FindComponentByClass<USFBuildComponent>())
	{
		Build->AddMaterial(Material, Amount);
	}
}

float ASFResourceNode::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Applied <= 0.f)
	{
		return 0.f;
	}

	// The harvester is whoever caused the damage — DamageCauser is the shooting
	// pawn, since the weapon component passes its owner.
	AActor* Harvester = DamageCauser;
	if (!Harvester && EventInstigator)
	{
		Harvester = EventInstigator->GetPawn();
	}

	NodeHealth -= Applied;
	DamageAccumulator += Applied;

	// Pay out in chunks rather than per point of damage, so a slow weapon and a
	// fast one yield the same materials for the same damage dealt.
	const float Step = FMath::Max(DamagePerYield, 1.f);
	while (DamageAccumulator >= Step)
	{
		DamageAccumulator -= Step;
		GrantMaterials(Harvester, SFBuild::HarvestYield(Material));
	}

	if (NodeHealth <= 0.f)
	{
		// Final partial swing still pays, so depleting a node never wastes damage.
		if (DamageAccumulator > 0.f)
		{
			GrantMaterials(Harvester, SFBuild::HarvestYield(Material));
		}
		Destroy();
	}
	return Applied;
}
