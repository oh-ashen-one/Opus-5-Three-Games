// Copyright Opus 5 Three Games. Original work.

#include "SFBuildComponent.h"
#include "SFStructure.h"
#include "SFBuildRegistry.h"

#include "GameFramework/Pawn.h"

namespace
{
	constexpr int32 NumMaterials = 3;

	int32 MaterialIndex(ESFBuildMaterial Material)
	{
		return FMath::Clamp(static_cast<int32>(Material), 0, NumMaterials - 1);
	}
}

USFBuildComponent::USFBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFBuildComponent::EnsureCarriedSized()
{
	if (Carried.Num() != NumMaterials)
	{
		Carried.SetNumZeroed(NumMaterials);
	}
}

void USFBuildComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureCarriedSized();
	AddMaterial(ESFBuildMaterial::Wood, StartingWood);
}

void USFBuildComponent::AddMaterial(ESFBuildMaterial InMaterial, int32 Amount)
{
	EnsureCarriedSized();
	const int32 Index = MaterialIndex(InMaterial);
	Carried[Index] = FMath::Clamp(Carried[Index] + Amount, 0, CarryCap);
}

int32 USFBuildComponent::GetMaterial(ESFBuildMaterial InMaterial) const
{
	if (Carried.Num() != NumMaterials)
	{
		return 0;
	}
	return Carried[MaterialIndex(InMaterial)];
}

void USFBuildComponent::CyclePiece(int32 Delta)
{
	constexpr int32 NumPieces = 4;
	int32 Index = static_cast<int32>(SelectedPiece) + Delta;
	Index = ((Index % NumPieces) + NumPieces) % NumPieces;
	SelectedPiece = static_cast<ESFBuildPiece>(Index);
}

bool USFBuildComponent::TryBuild()
{
	EnsureCarriedSized();

	APawn* Owner = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return false;
	}

	USFBuildRegistry* Registry = World->GetSubsystem<USFBuildRegistry>();
	if (!Registry)
	{
		return false;
	}

	const int32 Cost = SFBuild::CostFor(SelectedMaterial);
	const int32 Index = MaterialIndex(SelectedMaterial);
	if (Carried[Index] < Cost)
	{
		return false;
	}

	const FSFBuildSlot Slot = SFBuild::ResolveTargetSlot(
		Owner->GetActorLocation(),
		Owner->GetActorRotation().Yaw,
		SelectedPiece);

	if (Registry->IsOccupied(Slot))
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASFStructure* Piece = World->SpawnActor<ASFStructure>(
		ASFStructure::StaticClass(), FTransform::Identity, Params);
	if (!Piece)
	{
		return false;
	}

	Piece->InitPiece(Slot, SelectedMaterial);

	if (!Registry->Claim(Slot, Piece))
	{
		// Lost a race for the slot; don't charge the player for nothing.
		Piece->Destroy();
		return false;
	}

	Carried[Index] -= Cost;
	return true;
}
