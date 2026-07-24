// Copyright Opus 5 Three Games. Original work.

#include "Misc/AutomationTest.h"
#include "SFBotBrain.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** A calm, healthy, armed bot standing safely in the middle of the circle. */
	FSFBotContext CalmBot()
	{
		FSFBotContext C;
		C.Health = 100.f;
		C.Shield = 50.f;
		C.bHasWeapon = true;
		C.AmmoInMag = 30;
		C.bReloading = false;
		C.DistanceInsideStorm = 8000.f;
		C.TimeUntilShrink = 90.f;
		C.DistanceToSafety = 0.f;
		C.bEnemyVisible = false;
		C.DistanceToEnemy = 100000.f;
		C.TimeSinceDamaged = 999.f;
		C.BuildMaterials = 200;
		C.bLootKnown = false;
		C.DistanceToLoot = 100000.f;
		return C;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFBotRotationTest,
	"Stormfall.Bot.Rotation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFBotRotationTest::RunTest(const FString& Parameters)
{
	const FSFBotTuning T;

	// Caught outside the circle: leaving is the only acceptable answer.
	{
		FSFBotContext C = CalmBot();
		C.DistanceInsideStorm = -500.f;
		C.DistanceToSafety = 4000.f;
		TestTrue(TEXT("outside the storm must rotate"), SFBotBrain::MustRotateNow(C, T));
		TestEqual(TEXT("outside the storm rotates"), SFBotBrain::Decide(C, T), ESFBotAction::RotateToSafety);
	}

	// A bot that wins a fight and then dies to the circle looks broken, so the
	// storm must outrank an enemy in view.
	{
		FSFBotContext C = CalmBot();
		C.DistanceInsideStorm = -100.f;
		C.DistanceToSafety = 5000.f;
		C.bEnemyVisible = true;
		C.DistanceToEnemy = 1500.f;
		TestEqual(TEXT("storm outranks a visible enemy"),
			SFBotBrain::Decide(C, T), ESFBotAction::RotateToSafety);
	}

	// ...unless it's low and actively being shot, in which case wall up first.
	{
		FSFBotContext C = CalmBot();
		C.DistanceInsideStorm = -100.f;
		C.DistanceToSafety = 5000.f;
		C.bEnemyVisible = true;
		C.Health = 20.f;
		C.TimeSinceDamaged = 0.3f;
		TestEqual(TEXT("wall up before running while low and under fire"),
			SFBotBrain::Decide(C, T), ESFBotAction::PanicBuild);
	}

	// Timing: far from safety with the circle about to close means leave now.
	{
		FSFBotContext C = CalmBot();
		C.DistanceInsideStorm = 3000.f;
		C.DistanceToSafety = 30000.f; // 50s of travel at 600cm/s
		C.TimeUntilShrink = 40.f;
		TestTrue(TEXT("long trip with little time means leave now"), SFBotBrain::MustRotateNow(C, T));
	}

	// Close to safety with plenty of time: no need to move yet.
	{
		FSFBotContext C = CalmBot();
		C.DistanceInsideStorm = 3000.f;
		C.DistanceToSafety = 1000.f;
		C.TimeUntilShrink = 90.f;
		TestFalse(TEXT("short trip with lots of time can wait"), SFBotBrain::MustRotateNow(C, T));
	}

	// The safety factor must actually leave slack: a trip that exactly fills the
	// timer should still count as "go now".
	{
		FSFBotContext C = CalmBot();
		C.DistanceInsideStorm = 3000.f;
		C.DistanceToSafety = 600.f * 30.f; // exactly 30s of travel
		C.TimeUntilShrink = 35.f;          // only 5s of slack
		TestTrue(TEXT("safety factor forces an early departure"), SFBotBrain::MustRotateNow(C, T));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFBotCombatTest,
	"Stormfall.Bot.Combat",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFBotCombatTest::RunTest(const FString& Parameters)
{
	const FSFBotTuning T;

	// Healthy, armed, enemy in range: fight.
	{
		FSFBotContext C = CalmBot();
		C.bEnemyVisible = true;
		C.DistanceToEnemy = 2000.f;
		TestEqual(TEXT("healthy bot engages"), SFBotBrain::Decide(C, T), ESFBotAction::Engage);
	}

	// Enemy visible but way out of range: don't plink pointlessly.
	{
		FSFBotContext C = CalmBot();
		C.bEnemyVisible = true;
		C.DistanceToEnemy = T.EngageRange * 3.f;
		TestNotEqual(TEXT("does not shoot at nothing"), SFBotBrain::Decide(C, T), ESFBotAction::Engage);
	}

	// Low health under fire with materials: build. This is the behaviour that
	// makes bots read as Fortnite bots rather than generic shooter bots.
	{
		FSFBotContext C = CalmBot();
		C.bEnemyVisible = true;
		C.DistanceToEnemy = 1500.f;
		C.Health = 25.f;
		C.TimeSinceDamaged = 0.5f;
		TestEqual(TEXT("panic builds when hurt"), SFBotBrain::Decide(C, T), ESFBotAction::PanicBuild);
	}

	// Same, but out of materials: fall back to cover rather than doing nothing.
	{
		FSFBotContext C = CalmBot();
		C.bEnemyVisible = true;
		C.DistanceToEnemy = 1500.f;
		C.Health = 25.f;
		C.TimeSinceDamaged = 0.5f;
		C.BuildMaterials = 0;
		TestEqual(TEXT("takes cover with no materials"), SFBotBrain::Decide(C, T), ESFBotAction::TakeCover);
	}

	// Hurt but nobody has shot recently: no panic, keep playing.
	{
		FSFBotContext C = CalmBot();
		C.Health = 20.f;
		C.TimeSinceDamaged = 30.f;
		C.bEnemyVisible = true;
		C.DistanceToEnemy = 2000.f;
		TestEqual(TEXT("old damage does not trigger panic"), SFBotBrain::Decide(C, T), ESFBotAction::Engage);
	}

	// Empty magazine in a fight: reload rather than dry-firing forever.
	{
		FSFBotContext C = CalmBot();
		C.bEnemyVisible = true;
		C.DistanceToEnemy = 2000.f;
		C.AmmoInMag = 0;
		TestEqual(TEXT("reloads when empty"), SFBotBrain::Decide(C, T), ESFBotAction::Reload);
	}

	// Caught mid-reload with someone shooting: cover, don't stand there.
	{
		FSFBotContext C = CalmBot();
		C.bEnemyVisible = true;
		C.DistanceToEnemy = 2000.f;
		C.bReloading = true;
		const ESFBotAction A = SFBotBrain::Decide(C, T);
		TestTrue(TEXT("protects itself mid-reload"),
			A == ESFBotAction::PanicBuild || A == ESFBotAction::TakeCover);
	}

	// Unarmed and facing someone: do not charge. Loot if possible.
	{
		FSFBotContext C = CalmBot();
		C.bHasWeapon = false;
		C.bEnemyVisible = true;
		C.DistanceToEnemy = 2000.f;
		C.bLootKnown = true;
		TestEqual(TEXT("unarmed bot goes for loot, not the enemy"),
			SFBotBrain::Decide(C, T), ESFBotAction::Loot);
	}

	// Topping up between fights.
	{
		FSFBotContext C = CalmBot();
		C.AmmoInMag = 1;
		TestEqual(TEXT("reloads between fights"), SFBotBrain::Decide(C, T), ESFBotAction::Reload);
	}

	// Never returns an action it has no means to perform.
	{
		FSFBotContext C = CalmBot();
		C.bHasWeapon = false;
		C.bLootKnown = false;
		C.BuildMaterials = 0;
		const ESFBotAction A = SFBotBrain::Decide(C, T);
		TestNotEqual(TEXT("never engages without a weapon"), A, ESFBotAction::Engage);
		TestNotEqual(TEXT("never reloads without a weapon"), A, ESFBotAction::Reload);
		TestNotEqual(TEXT("never loots with no loot known"), A, ESFBotAction::Loot);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
