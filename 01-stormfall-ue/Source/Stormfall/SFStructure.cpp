// Copyright Opus 5 Three Games. Original work.

#include "SFStructure.h"
#include "SFBuildRegistry.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ASFStructure::ASFStructure()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECC_WorldStatic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Engine's unit cube is 100cm; all piece geometry is scale on this one mesh.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CubeFinder.Object);
	}

	SetCanBeDamaged(true);
}

void ASFStructure::InitPiece(const FSFBuildSlot& InSlot, ESFBuildMaterial InMaterial)
{
	Slot = InSlot;
	Material = InMaterial;
	MaxHealth = SFBuild::MaxHealthFor(InMaterial);
	Health = MaxHealth;

	const float Cell = SFBuild::GridSize;
	const float Units = Cell / 100.f; // cube is 100cm
	const float Thickness = 0.2f;
	const FVector Base = SFBuild::CellToWorld(Slot.Cell, Cell);
	const float Yaw = SFBuild::QuarterTurnsToYaw(Slot.QuarterTurns);

	FVector Location = Base;
	FRotator Rotation(0.f, Yaw, 0.f);
	FVector Scale(Units, Units, Units);

	switch (Slot.Piece)
	{
		case ESFBuildPiece::Wall:
			// Thin in local Y, standing on the cell floor.
			Scale = FVector(Thickness, Units, Units);
			Location.Z = Base.Z + Cell * 0.5f;
			break;

		case ESFBuildPiece::Floor:
			Scale = FVector(Units, Units, Thickness);
			Location.Z = Base.Z + Cell * Thickness * 0.5f;
			break;

		case ESFBuildPiece::Roof:
			Scale = FVector(Units, Units, Thickness);
			Location.Z = Base.Z + Cell - Cell * Thickness * 0.5f;
			break;

		case ESFBuildPiece::Ramp:
			// A slab pitched 45 degrees, long enough to span the cell diagonally.
			Scale = FVector(Units * 1.42f, Units, Thickness);
			Rotation = FRotator(45.f, Yaw, 0.f);
			Location.Z = Base.Z + Cell * 0.5f;
			break;
	}

	SetActorLocationAndRotation(Location, Rotation);
	Mesh->SetWorldScale3D(Scale);
}

float ASFStructure::TakeDamage(
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

	Health = FMath::Max(Health - Applied, 0.f);
	if (Health <= 0.f)
	{
		Destroy();
	}
	return Applied;
}

void ASFStructure::EndPlay(const EEndPlayReason::Type Reason)
{
	// Free the slot so the cell can be rebuilt immediately — critical in a fight.
	if (UWorld* World = GetWorld())
	{
		if (USFBuildRegistry* Registry = World->GetSubsystem<USFBuildRegistry>())
		{
			Registry->Release(Slot);
		}
	}
	Super::EndPlay(Reason);
}
