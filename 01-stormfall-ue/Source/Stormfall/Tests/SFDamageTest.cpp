// Copyright Opus 5 Three Games. Original work.
//
// Headless unit tests. Run with:
//   UnrealEditor-Cmd Stormfall.uproject -unattended -nullrhi -nosplash \
//     -ExecCmds="Automation RunTests Stormfall; quit" -TestExit="Automation Test Queue Empty"

#include "Misc/AutomationTest.h"
#include "SFDamage.h"

#if WITH_DEV_AUTOMATION_TESTS

// ApplicationContextMask rather than EditorContext: these are pure-math tests with
// no context requirement, and the headless runner is a commandlet, not the editor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFFallDamageTest,
	"Stormfall.Damage.FallDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFFallDamageTest::RunTest(const FString& Parameters)
{
	const float Safe = 1200.f;
	const float Lethal = 2600.f;
	const float MaxHP = 100.f;

	// Below and at the safe threshold: landing is free.
	TestEqual(TEXT("standing still"), SFDamage::ComputeFallDamage(0.f, Safe, Lethal, MaxHP), 0.f);
	TestEqual(TEXT("short hop"), SFDamage::ComputeFallDamage(600.f, Safe, Lethal, MaxHP), 0.f);
	TestEqual(TEXT("exactly at safe speed"), SFDamage::ComputeFallDamage(Safe, Safe, Lethal, MaxHP), 0.f);

	// Just past the threshold should sting, not gut you. Guards against an
	// off-by-one where the ramp starts at full damage.
	const float JustOver = SFDamage::ComputeFallDamage(Safe + 1.f, Safe, Lethal, MaxHP);
	TestTrue(TEXT("just past safe is nearly free"), JustOver > 0.f && JustOver < 1.f);

	// Midpoint of the window is half of max health.
	TestNearlyEqual(
		TEXT("midpoint is half health"),
		SFDamage::ComputeFallDamage((Safe + Lethal) * 0.5f, Safe, Lethal, MaxHP),
		MaxHP * 0.5f,
		0.01f);

	// At and beyond lethal speed, damage saturates rather than overshooting.
	TestEqual(TEXT("lethal speed"), SFDamage::ComputeFallDamage(Lethal, Safe, Lethal, MaxHP), MaxHP);
	TestEqual(TEXT("terminal velocity clamps"), SFDamage::ComputeFallDamage(99999.f, Safe, Lethal, MaxHP), MaxHP);

	// Monotonic across the whole window — no dips where falling further hurts less.
	float Previous = -1.f;
	for (float Speed = 0.f; Speed <= 4000.f; Speed += 50.f)
	{
		const float Damage = SFDamage::ComputeFallDamage(Speed, Safe, Lethal, MaxHP);
		TestTrue(TEXT("damage never decreases as impact speed rises"), Damage >= Previous);
		TestTrue(TEXT("damage never exceeds max health"), Damage <= MaxHP);
		Previous = Damage;
	}

	// Degenerate window: anything past safe is lethal, and nothing divides by zero.
	TestEqual(TEXT("zero-width window is lethal"), SFDamage::ComputeFallDamage(1300.f, 1200.f, 1200.f, MaxHP), MaxHP);
	TestEqual(TEXT("zero-width window still safe below"), SFDamage::ComputeFallDamage(900.f, 1200.f, 1200.f, MaxHP), 0.f);
	TestEqual(TEXT("inverted window is lethal"), SFDamage::ComputeFallDamage(1300.f, 1200.f, 800.f, MaxHP), MaxHP);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
