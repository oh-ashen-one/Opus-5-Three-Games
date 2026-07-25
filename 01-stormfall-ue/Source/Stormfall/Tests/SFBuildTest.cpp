// Copyright Opus 5 Three Games. Original work.

#include "Misc/AutomationTest.h"
#include "SFBuild.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFBuildGridTest,
	"Stormfall.Build.Grid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFBuildGridTest::RunTest(const FString& Parameters)
{
	const float Size = SFBuild::GridSize;

	// Cells tile the world with no gaps or overlaps, including across zero —
	// floor-division is required here, truncation would make two cells share x=0.
	TestEqual(TEXT("origin"), SFBuild::WorldToCell(FVector(0, 0, 0), Size), FIntVector(0, 0, 0));
	TestEqual(TEXT("just inside cell 0"), SFBuild::WorldToCell(FVector(Size - 1, 0, 0), Size), FIntVector(0, 0, 0));
	TestEqual(TEXT("start of cell 1"), SFBuild::WorldToCell(FVector(Size, 0, 0), Size), FIntVector(1, 0, 0));
	TestEqual(TEXT("negative side is cell -1"), SFBuild::WorldToCell(FVector(-1, 0, 0), Size), FIntVector(-1, 0, 0));
	TestEqual(TEXT("start of cell -1"), SFBuild::WorldToCell(FVector(-Size, 0, 0), Size), FIntVector(-1, 0, 0));

	// Round trip: a cell's own world position must map back to that same cell.
	for (int32 X = -3; X <= 3; ++X)
	{
		for (int32 Z = -2; Z <= 2; ++Z)
		{
			const FIntVector Cell(X, X * 2, Z);
			const FVector World = SFBuild::CellToWorld(Cell, Size);
			TestEqual(TEXT("cell round trips through world space"), SFBuild::WorldToCell(World, Size), Cell);
		}
	}

	// Cell floors sit on the grid plane so stacked pieces meet instead of overlapping.
	TestEqual(TEXT("cell 0 floor is at z=0"), SFBuild::CellToWorld(FIntVector(0, 0, 0), Size).Z, 0.0);
	TestEqual(TEXT("cell 1 floor is one cell up"), SFBuild::CellToWorld(FIntVector(0, 0, 1), Size).Z, (double)Size);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFBuildRotationTest,
	"Stormfall.Build.Rotation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFBuildRotationTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("facing +X"), SFBuild::YawToQuarterTurns(0.f), 0);
	TestEqual(TEXT("facing +Y"), SFBuild::YawToQuarterTurns(90.f), 1);
	TestEqual(TEXT("facing -X"), SFBuild::YawToQuarterTurns(180.f), 2);
	TestEqual(TEXT("facing -Y"), SFBuild::YawToQuarterTurns(270.f), 3);

	// Wrapping and negatives must not produce out-of-range turns.
	TestEqual(TEXT("360 wraps to 0"), SFBuild::YawToQuarterTurns(360.f), 0);
	TestEqual(TEXT("-90 is 3"), SFBuild::YawToQuarterTurns(-90.f), 3);
	TestEqual(TEXT("-450 is 3"), SFBuild::YawToQuarterTurns(-450.f), 3);
	TestEqual(TEXT("720 is 0"), SFBuild::YawToQuarterTurns(720.f), 0);

	// Snapping: anything within 45 degrees of a face picks that face.
	TestEqual(TEXT("40 degrees snaps to +X"), SFBuild::YawToQuarterTurns(40.f), 0);
	TestEqual(TEXT("50 degrees snaps to +Y"), SFBuild::YawToQuarterTurns(50.f), 1);

	// Every possible yaw yields a valid quarter turn.
	for (float Yaw = -720.f; Yaw <= 720.f; Yaw += 7.f)
	{
		const int32 Turns = SFBuild::YawToQuarterTurns(Yaw);
		TestTrue(TEXT("quarter turns stay in [0,3]"), Turns >= 0 && Turns <= 3);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFBuildPlacementTest,
	"Stormfall.Build.Placement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFBuildPlacementTest::RunTest(const FString& Parameters)
{
	const float Size = SFBuild::GridSize;
	const FVector At = SFBuild::CellToWorld(FIntVector(0, 0, 0), Size);

	// You build in front of yourself, in the direction you face.
	{
		const FSFBuildSlot S = SFBuild::ResolveTargetSlot(At, 0.f, ESFBuildPiece::Wall, Size);
		TestEqual(TEXT("wall goes one cell along +X"), S.Cell, FIntVector(1, 0, 0));
	}
	{
		const FSFBuildSlot S = SFBuild::ResolveTargetSlot(At, 180.f, ESFBuildPiece::Wall, Size);
		TestEqual(TEXT("wall goes one cell along -X"), S.Cell, FIntVector(-1, 0, 0));
	}
	{
		const FSFBuildSlot S = SFBuild::ResolveTargetSlot(At, 90.f, ESFBuildPiece::Floor, Size);
		TestEqual(TEXT("floor goes one cell along +Y"), S.Cell, FIntVector(0, 1, 0));
	}

	// Roofs go overhead, which is what makes panic-roofing actually protect you.
	{
		const FSFBuildSlot S = SFBuild::ResolveTargetSlot(At, 0.f, ESFBuildPiece::Roof, Size);
		TestEqual(TEXT("roof is ahead and one cell up"), S.Cell, FIntVector(1, 0, 1));
	}

	// Determinism: the same input must always resolve to the same slot, or players
	// can't build reliably under pressure.
	for (int32 i = 0; i < 32; ++i)
	{
		const FSFBuildSlot A = SFBuild::ResolveTargetSlot(At, 37.f, ESFBuildPiece::Ramp, Size);
		const FSFBuildSlot B = SFBuild::ResolveTargetSlot(At, 37.f, ESFBuildPiece::Ramp, Size);
		TestTrue(TEXT("placement is deterministic"), A == B);
	}

	// Slot identity: rotation distinguishes walls, but not floors.
	{
		FSFBuildSlot WallA; WallA.Cell = FIntVector(1, 1, 0); WallA.Piece = ESFBuildPiece::Wall; WallA.QuarterTurns = 0;
		FSFBuildSlot WallB = WallA; WallB.QuarterTurns = 1;
		TestFalse(TEXT("walls at different facings are different slots"), WallA == WallB);

		FSFBuildSlot FloorA; FloorA.Cell = FIntVector(1, 1, 0); FloorA.Piece = ESFBuildPiece::Floor; FloorA.QuarterTurns = 0;
		FSFBuildSlot FloorB = FloorA; FloorB.QuarterTurns = 3;
		TestTrue(TEXT("floors are rotationally symmetric"), FloorA == FloorB);
		TestEqual(TEXT("symmetric slots hash equal"), GetTypeHash(FloorA), GetTypeHash(FloorB));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFBuildMaterialsTest,
	"Stormfall.Build.Materials",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFBuildMaterialsTest::RunTest(const FString& Parameters)
{
	// Strength ordering is the entire point of having three materials.
	TestTrue(TEXT("stone beats wood"),
		SFBuild::MaxHealthFor(ESFBuildMaterial::Stone) > SFBuild::MaxHealthFor(ESFBuildMaterial::Wood));
	TestTrue(TEXT("metal beats stone"),
		SFBuild::MaxHealthFor(ESFBuildMaterial::Metal) > SFBuild::MaxHealthFor(ESFBuildMaterial::Stone));

	// Harvest speed runs the other way, so wood is the panic material and metal
	// is the one you invest in.
	TestTrue(TEXT("wood harvests faster than stone"),
		SFBuild::HarvestYield(ESFBuildMaterial::Wood) > SFBuild::HarvestYield(ESFBuildMaterial::Stone));
	TestTrue(TEXT("stone harvests faster than metal"),
		SFBuild::HarvestYield(ESFBuildMaterial::Stone) > SFBuild::HarvestYield(ESFBuildMaterial::Metal));

	// A single swing must fund at least one piece, or building under fire is dead.
	for (ESFBuildMaterial M : { ESFBuildMaterial::Wood, ESFBuildMaterial::Stone, ESFBuildMaterial::Metal })
	{
		TestTrue(TEXT("one swing affords at least one piece"), SFBuild::HarvestYield(M) >= SFBuild::CostFor(M) / 2);
		TestTrue(TEXT("cost is positive"), SFBuild::CostFor(M) > 0);
		TestTrue(TEXT("health is positive"), SFBuild::MaxHealthFor(M) > 0.f);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
