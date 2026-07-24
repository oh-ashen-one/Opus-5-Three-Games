// Copyright Opus 5 Three Games. Original work.

#include "SFBuildRegistry.h"
#include "SFStructure.h"

bool USFBuildRegistry::IsOccupied(const FSFBuildSlot& Slot) const
{
	const TObjectPtr<ASFStructure>* Found = Occupancy.Find(Slot);
	if (Found == nullptr)
	{
		return false;
	}

	// IsValid rather than a null check: an actor that has been destroyed is still
	// a non-null pointer until it is collected. Relying on EndPlay to release the
	// slot leaves the cell blocked for that window — and EndPlay never fires at
	// all in an editor world. Either way the player is standing there unable to
	// rebuild a wall that visibly no longer exists.
	return IsValid(*Found);
}

bool USFBuildRegistry::Claim(const FSFBuildSlot& Slot, ASFStructure* Piece)
{
	if (IsOccupied(Slot))
	{
		return false;
	}
	// Add overwrites any stale entry left by a destroyed piece.
	Occupancy.Add(Slot, Piece);
	return true;
}

void USFBuildRegistry::Release(const FSFBuildSlot& Slot)
{
	Occupancy.Remove(Slot);
}
