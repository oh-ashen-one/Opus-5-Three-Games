// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SFBuild.h"
#include "SFBuildRegistry.generated.h"

class ASFStructure;

/**
 * Tracks which build slots are occupied, so a cell can't hold two of the same
 * piece and so a destroyed piece frees its slot for an immediate rebuild.
 *
 * A world subsystem rather than state on the character: structures outlive the
 * player who built them, and bots build into the same grid.
 */
UCLASS()
class STORMFALL_API USFBuildRegistry : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** True if something already occupies this slot. */
	UFUNCTION(BlueprintPure, Category = "Stormfall|Build")
	bool IsOccupied(const FSFBuildSlot& Slot) const;

	/** Claim a slot. Returns false if it was already taken. */
	bool Claim(const FSFBuildSlot& Slot, ASFStructure* Piece);

	/** Release a slot, e.g. when its piece is destroyed. */
	void Release(const FSFBuildSlot& Slot);

	UFUNCTION(BlueprintPure, Category = "Stormfall|Build")
	int32 NumPlaced() const { return Occupancy.Num(); }

private:
	UPROPERTY()
	TMap<FSFBuildSlot, TObjectPtr<ASFStructure>> Occupancy;
};
