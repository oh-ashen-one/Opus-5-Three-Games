// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "SFBuild.generated.h"

/** The four buildable pieces. Deliberately the classic set — anything more is noise. */
UENUM(BlueprintType)
enum class ESFBuildPiece : uint8
{
	Wall	UMETA(DisplayName = "Wall"),
	Ramp	UMETA(DisplayName = "Ramp"),
	Floor	UMETA(DisplayName = "Floor"),
	Roof	UMETA(DisplayName = "Roof"),
};

/** Harvestable materials, weakest to strongest. */
UENUM(BlueprintType)
enum class ESFBuildMaterial : uint8
{
	Wood	UMETA(DisplayName = "Wood"),
	Stone	UMETA(DisplayName = "Stone"),
	Metal	UMETA(DisplayName = "Metal"),
};

/** A placement slot: which cell, which face/orientation, which piece. */
USTRUCT(BlueprintType)
struct FSFBuildSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Build")
	FIntVector Cell = FIntVector::ZeroValue;

	UPROPERTY(BlueprintReadWrite, Category = "Build")
	ESFBuildPiece Piece = ESFBuildPiece::Wall;

	/** Quarter turns about Z. Only meaningful for walls and ramps. */
	UPROPERTY(BlueprintReadWrite, Category = "Build")
	int32 QuarterTurns = 0;

	bool operator==(const FSFBuildSlot& Other) const
	{
		// Floors and roofs are rotationally symmetric for occupancy purposes, so
		// two floors in the same cell collide regardless of facing.
		const bool bRotationMatters = (Piece == ESFBuildPiece::Wall || Piece == ESFBuildPiece::Ramp);
		return Cell == Other.Cell
			&& Piece == Other.Piece
			&& (!bRotationMatters || QuarterTurns == Other.QuarterTurns);
	}
};

FORCEINLINE uint32 GetTypeHash(const FSFBuildSlot& Slot)
{
	const bool bRotationMatters = (Slot.Piece == ESFBuildPiece::Wall || Slot.Piece == ESFBuildPiece::Ramp);
	uint32 Hash = GetTypeHash(Slot.Cell);
	Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Slot.Piece)));
	if (bRotationMatters)
	{
		Hash = HashCombine(Hash, GetTypeHash(Slot.QuarterTurns));
	}
	return Hash;
}

/**
 * Build grid and material rules, as pure functions. Placement has to be
 * predictable to the player — the same look direction from the same spot must
 * always target the same cell — so all of it is deterministic and unit tested.
 */
namespace SFBuild
{
	/** Cell size in cm. One wall is one cell wide. */
	inline constexpr float GridSize = 400.f;

	/** Which cell a world location falls in. */
	STORMFALL_API FIntVector WorldToCell(const FVector& World, float CellSize = GridSize);

	/** World location of a cell's floor center. */
	STORMFALL_API FVector CellToWorld(const FIntVector& Cell, float CellSize = GridSize);

	/** Quarter turns (0-3) for a yaw in degrees, snapped to the nearest face. */
	STORMFALL_API int32 YawToQuarterTurns(float YawDegrees);

	/** Yaw in degrees for a quarter-turn count. */
	STORMFALL_API float QuarterTurnsToYaw(int32 QuarterTurns);

	/**
	 * The slot a player targets when they build. Walls go on the face they're
	 * facing, ramps and floors go in the cell ahead, roofs go overhead.
	 */
	STORMFALL_API FSFBuildSlot ResolveTargetSlot(
		const FVector& PlayerLocation,
		float PlayerYawDegrees,
		ESFBuildPiece Piece,
		float CellSize = GridSize);

	/** Structural health for a piece built from a material. */
	STORMFALL_API float MaxHealthFor(ESFBuildMaterial Material);

	/** Resource cost to place one piece. Uniform across piece types, like the original. */
	STORMFALL_API int32 CostFor(ESFBuildMaterial Material);

	/** How much of a material one pickaxe swing yields from a matching resource node. */
	STORMFALL_API int32 HarvestYield(ESFBuildMaterial Material);
}
