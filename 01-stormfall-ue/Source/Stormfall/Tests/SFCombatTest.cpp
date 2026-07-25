// Copyright Opus 5 Three Games. Original work.

#include "Misc/AutomationTest.h"
#include "SFCombat.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFCombatFalloffTest,
	"Stormfall.Combat.Falloff",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFCombatFalloffTest::RunTest(const FString& Parameters)
{
	const FSFWeaponStats AR = SFCombat::MakeWeapon(ESFWeaponClass::AssaultRifle, ESFRarity::Common);

	// Full damage inside the falloff start.
	TestNearlyEqual(TEXT("point blank is full damage"),
		SFCombat::ComputeHitDamage(AR, 0.f, false), AR.BaseDamage, 0.01f);
	TestNearlyEqual(TEXT("at falloff start is still full"),
		SFCombat::ComputeHitDamage(AR, AR.FalloffStart, false), AR.BaseDamage, 0.01f);

	// Halfway through the falloff window is halfway to the minimum.
	{
		const float Mid = (AR.FalloffStart + AR.FalloffEnd) * 0.5f;
		const float Expected = AR.BaseDamage * (1.f + AR.MinDamageFraction) * 0.5f;
		TestNearlyEqual(TEXT("midpoint of falloff"), SFCombat::ComputeHitDamage(AR, Mid, false), Expected, 0.01f);
	}

	// Past falloff end it floors out rather than continuing to zero.
	TestNearlyEqual(TEXT("beyond falloff end floors"),
		SFCombat::ComputeHitDamage(AR, AR.FalloffEnd + 1000.f, false),
		AR.BaseDamage * AR.MinDamageFraction, 0.01f);

	// Beyond max range the bullet does nothing at all.
	TestEqual(TEXT("past max range does nothing"),
		SFCombat::ComputeHitDamage(AR, AR.MaxRange + 1.f, false), 0.f);

	// Headshots multiply, and never reduce damage.
	TestTrue(TEXT("headshot hurts more"),
		SFCombat::ComputeHitDamage(AR, 1000.f, true) > SFCombat::ComputeHitDamage(AR, 1000.f, false));

	// Damage must never increase with distance, and never go negative.
	{
		float Previous = TNumericLimits<float>::Max();
		for (float D = 0.f; D <= AR.MaxRange; D += 250.f)
		{
			const float Dmg = SFCombat::ComputeHitDamage(AR, D, false);
			TestTrue(TEXT("damage never rises with distance"), Dmg <= Previous + 0.01f);
			TestTrue(TEXT("damage never negative"), Dmg >= 0.f);
			Previous = Dmg;
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFCombatWeaponsTest,
	"Stormfall.Combat.Weapons",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFCombatWeaponsTest::RunTest(const FString& Parameters)
{
	// Rarity must be strictly better each tier, or the loot chase is meaningless.
	const TArray<ESFRarity> Tiers = {
		ESFRarity::Common, ESFRarity::Uncommon, ESFRarity::Rare, ESFRarity::Epic, ESFRarity::Legendary };
	for (int32 i = 1; i < Tiers.Num(); ++i)
	{
		TestTrue(TEXT("rarity multiplier strictly increases"),
			SFCombat::RarityMultiplier(Tiers[i]) > SFCombat::RarityMultiplier(Tiers[i - 1]));

		const FSFWeaponStats Lower = SFCombat::MakeWeapon(ESFWeaponClass::AssaultRifle, Tiers[i - 1]);
		const FSFWeaponStats Higher = SFCombat::MakeWeapon(ESFWeaponClass::AssaultRifle, Tiers[i]);
		TestTrue(TEXT("higher rarity hits harder"), Higher.BaseDamage > Lower.BaseDamage);
		TestTrue(TEXT("higher rarity reloads faster"), Higher.ReloadSeconds < Lower.ReloadSeconds);
	}

	// A Common weapon must stay competitive — the gap is a nudge, not a wall.
	{
		const FSFWeaponStats Common = SFCombat::MakeWeapon(ESFWeaponClass::AssaultRifle, ESFRarity::Common);
		const FSFWeaponStats Legendary = SFCombat::MakeWeapon(ESFWeaponClass::AssaultRifle, ESFRarity::Legendary);
		TestTrue(TEXT("legendary is under 1.6x a common"), Legendary.BaseDamage < Common.BaseDamage * 1.6f);
	}

	// Class identity: the archetypes must actually behave differently.
	{
		const FSFWeaponStats Shotgun = SFCombat::MakeWeapon(ESFWeaponClass::Shotgun, ESFRarity::Common);
		const FSFWeaponStats Sniper = SFCombat::MakeWeapon(ESFWeaponClass::Sniper, ESFRarity::Common);
		const FSFWeaponStats SMG = SFCombat::MakeWeapon(ESFWeaponClass::SMG, ESFRarity::Common);

		TestTrue(TEXT("shotgun fires multiple pellets"), Shotgun.Pellets > 1);
		TestTrue(TEXT("shotgun is short range"), Shotgun.MaxRange < Sniper.MaxRange);
		TestTrue(TEXT("sniper hits hardest per shot"), Sniper.BaseDamage > SMG.BaseDamage);
		TestTrue(TEXT("smg fires fastest"), SMG.FireRate > Sniper.FireRate);
		TestTrue(TEXT("sniper is the most accurate"), Sniper.SpreadDegrees < SMG.SpreadDegrees);

		// A close-range shotgun burst should be lethal-ish, or it isn't a shotgun.
		const float Burst = SFCombat::ComputeHitDamage(Shotgun, 300.f, false) * Shotgun.Pellets;
		TestTrue(TEXT("point blank shotgun burst is heavy"), Burst >= 80.f);
		// ...but not from across the map.
		const float FarBurst = SFCombat::ComputeHitDamage(Shotgun, 3000.f, false) * Shotgun.Pellets;
		TestTrue(TEXT("shotgun is weak at range"), FarBurst < 30.f);
	}

	// Fire rate to interval.
	{
		const FSFWeaponStats AR = SFCombat::MakeWeapon(ESFWeaponClass::AssaultRifle, ESFRarity::Common);
		TestNearlyEqual(TEXT("seconds per shot"), SFCombat::SecondsPerShot(AR), 1.f / AR.FireRate, 0.0001f);

		FSFWeaponStats Broken = AR;
		Broken.FireRate = 0.f;
		TestTrue(TEXT("zero fire rate does not divide by zero"), SFCombat::SecondsPerShot(Broken) > 0.f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFCombatShieldTest,
	"Stormfall.Combat.Shield",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFCombatShieldTest::RunTest(const FString& Parameters)
{
	// Shield absorbs first.
	{
		float Shield = 50.f, Health = 100.f;
		const float ToHealth = SFCombat::ApplyToShieldThenHealth(30.f, Shield, Health);
		TestEqual(TEXT("shield takes it"), Shield, 20.f);
		TestEqual(TEXT("health untouched"), Health, 100.f);
		TestEqual(TEXT("nothing reached health"), ToHealth, 0.f);
	}

	// Overflow spills into health, exactly once.
	{
		float Shield = 20.f, Health = 100.f;
		const float ToHealth = SFCombat::ApplyToShieldThenHealth(50.f, Shield, Health);
		TestEqual(TEXT("shield emptied"), Shield, 0.f);
		TestEqual(TEXT("health took the remainder"), Health, 70.f);
		TestEqual(TEXT("reported spill is right"), ToHealth, 30.f);
	}

	// Massive overkill must not drive health negative.
	{
		float Shield = 100.f, Health = 100.f;
		SFCombat::ApplyToShieldThenHealth(99999.f, Shield, Health);
		TestEqual(TEXT("shield floors at zero"), Shield, 0.f);
		TestEqual(TEXT("health floors at zero"), Health, 0.f);
	}

	// Zero and negative damage are no-ops rather than heals.
	{
		float Shield = 50.f, Health = 100.f;
		SFCombat::ApplyToShieldThenHealth(0.f, Shield, Health);
		SFCombat::ApplyToShieldThenHealth(-25.f, Shield, Health);
		TestEqual(TEXT("shield unchanged"), Shield, 50.f);
		TestEqual(TEXT("negative damage does not heal"), Health, 100.f);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
