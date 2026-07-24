// Copyright Opus 5 Three Games. Original work.

#include "SFBuild.h"

namespace SFBuild
{

FIntVector WorldToCell(const FVector& World, float CellSize)
{
	const float Size = FMath::Max(CellSize, 1.f);
	return FIntVector(
		FMath::FloorToInt(World.X / Size),
		FMath::FloorToInt(World.Y / Size),
		FMath::FloorToInt(World.Z / Size));
}

FVector CellToWorld(const FIntVector& Cell, float CellSize)
{
	const float Size = FMath::Max(CellSize, 1.f);
	// Centered in X/Y, but sitting on the floor of the cell in Z, so a wall's base
	// meets the floor piece below it instead of intersecting it.
	return FVector(
		(static_cast<float>(Cell.X) + 0.5f) * Size,
		(static_cast<float>(Cell.Y) + 0.5f) * Size,
		static_cast<float>(Cell.Z) * Size);
}

int32 YawToQuarterTurns(float YawDegrees)
{
	// Snap to the nearest 90 degrees, then wrap into [0,3].
	const float Normalized = FMath::UnwindDegrees(YawDegrees);
	int32 Turns = FMath::RoundToInt(Normalized / 90.f);
	Turns %= 4;
	if (Turns < 0)
	{
		Turns += 4;
	}
	return Turns;
}

float QuarterTurnsToYaw(int32 QuarterTurns)
{
	int32 Turns = QuarterTurns % 4;
	if (Turns < 0)
	{
		Turns += 4;
	}
	return static_cast<float>(Turns) * 90.f;
}

FSFBuildSlot ResolveTargetSlot(
	const FVector& PlayerLocation,
	float PlayerYawDegrees,
	ESFBuildPiece Piece,
	float CellSize)
{
	FSFBuildSlot Slot;
	Slot.Piece = Piece;
	Slot.QuarterTurns = YawToQuarterTurns(PlayerYawDegrees);

	const FIntVector Here = WorldToCell(PlayerLocation, CellSize);

	// Unit step in the facing direction, snapped to a cardinal axis.
	FIntVector Step(0, 0, 0);
	switch (Slot.QuarterTurns)
	{
		case 0: Step = FIntVector(1, 0, 0); break;   // +X
		case 1: Step = FIntVector(0, 1, 0); break;   // +Y
		case 2: Step = FIntVector(-1, 0, 0); break;  // -X
		default: Step = FIntVector(0, -1, 0); break; // -Y
	}

	switch (Piece)
	{
		case ESFBuildPiece::Wall:
		case ESFBuildPiece::Ramp:
		case ESFBuildPiece::Floor:
			// All go in the cell directly ahead: you build in front of yourself.
			Slot.Cell = Here + Step;
			break;

		case ESFBuildPiece::Roof:
			// Overhead and ahead, so panic-roofing covers you rather than the wall.
			Slot.Cell = Here + Step + FIntVector(0, 0, 1);
			break;
	}

	return Slot;
}

float MaxHealthFor(ESFBuildMaterial Material)
{
	switch (Material)
	{
		case ESFBuildMaterial::Wood:  return 150.f;
		case ESFBuildMaterial::Stone: return 300.f;
		case ESFBuildMaterial::Metal: return 500.f;
	}
	return 150.f;
}

int32 CostFor(ESFBuildMaterial Material)
{
	// Uniform cost keeps the decision about *which* material, not which piece.
	switch (Material)
	{
		case ESFBuildMaterial::Wood:  return 10;
		case ESFBuildMaterial::Stone: return 10;
		case ESFBuildMaterial::Metal: return 10;
	}
	return 10;
}

int32 HarvestYield(ESFBuildMaterial Material)
{
	// Wood comes fastest, metal slowest — that's the whole risk/reward of farming
	// metal late instead of throwing up wood immediately.
	switch (Material)
	{
		case ESFBuildMaterial::Wood:  return 12;
		case ESFBuildMaterial::Stone: return 9;
		case ESFBuildMaterial::Metal: return 6;
	}
	return 12;
}

} // namespace SFBuild
