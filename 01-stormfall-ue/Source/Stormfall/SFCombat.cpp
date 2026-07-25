// Copyright Opus 5 Three Games. Original work.

#include "SFCombat.h"

namespace SFCombat
{

float RarityMultiplier(ESFRarity Rarity)
{
	// ~10% per tier. Enough that finding a Legendary matters, small enough that
	// a Common in good hands still wins the fight.
	switch (Rarity)
	{
		case ESFRarity::Common:    return 1.00f;
		case ESFRarity::Uncommon:  return 1.10f;
		case ESFRarity::Rare:      return 1.21f;
		case ESFRarity::Epic:      return 1.33f;
		case ESFRarity::Legendary: return 1.46f;
	}
	return 1.f;
}

FString RarityName(ESFRarity Rarity)
{
	switch (Rarity)
	{
		case ESFRarity::Common:    return TEXT("Common");
		case ESFRarity::Uncommon:  return TEXT("Uncommon");
		case ESFRarity::Rare:      return TEXT("Rare");
		case ESFRarity::Epic:      return TEXT("Epic");
		case ESFRarity::Legendary: return TEXT("Legendary");
	}
	return TEXT("Common");
}

FSFWeaponStats MakeWeapon(ESFWeaponClass Class, ESFRarity Rarity)
{
	FSFWeaponStats S;
	S.Class = Class;
	S.Rarity = Rarity;

	switch (Class)
	{
		case ESFWeaponClass::Pistol:
			S.BaseDamage = 24.f;  S.FireRate = 6.5f;  S.MagazineSize = 16;
			S.ReloadSeconds = 1.4f; S.SpreadDegrees = 1.6f;
			S.FalloffStart = 2000.f; S.FalloffEnd = 6000.f; S.MinDamageFraction = 0.45f;
			S.MaxRange = 12000.f; S.HeadshotMultiplier = 2.0f;
			break;

		case ESFWeaponClass::AssaultRifle:
			S.BaseDamage = 31.f;  S.FireRate = 5.5f;  S.MagazineSize = 30;
			S.ReloadSeconds = 2.3f; S.SpreadDegrees = 1.2f;
			S.FalloffStart = 4000.f; S.FalloffEnd = 11000.f; S.MinDamageFraction = 0.5f;
			S.MaxRange = 25000.f; S.HeadshotMultiplier = 2.0f;
			break;

		case ESFWeaponClass::SMG:
			S.BaseDamage = 18.f;  S.FireRate = 11.f;  S.MagazineSize = 35;
			S.ReloadSeconds = 2.0f; S.SpreadDegrees = 2.6f;
			S.FalloffStart = 1600.f; S.FalloffEnd = 5000.f; S.MinDamageFraction = 0.35f;
			S.MaxRange = 10000.f; S.HeadshotMultiplier = 1.75f;
			break;

		case ESFWeaponClass::Shotgun:
			// Damage is per pellet; the burst is what hurts, and only up close.
			S.BaseDamage = 11.f;  S.FireRate = 1.3f;  S.MagazineSize = 6;
			S.ReloadSeconds = 3.0f; S.SpreadDegrees = 6.5f; S.Pellets = 9;
			S.FalloffStart = 500.f; S.FalloffEnd = 2200.f; S.MinDamageFraction = 0.12f;
			S.MaxRange = 4000.f; S.HeadshotMultiplier = 1.5f;
			break;

		case ESFWeaponClass::Sniper:
			// One shot, one decision. No falloff to speak of, brutal headshot.
			S.BaseDamage = 90.f;  S.FireRate = 0.55f; S.MagazineSize = 5;
			S.ReloadSeconds = 3.4f; S.SpreadDegrees = 0.15f;
			S.FalloffStart = 20000.f; S.FalloffEnd = 40000.f; S.MinDamageFraction = 0.85f;
			S.MaxRange = 60000.f; S.HeadshotMultiplier = 2.5f;
			break;
	}

	const float Mult = RarityMultiplier(Rarity);
	S.BaseDamage *= Mult;
	// Higher rarity also reloads a little faster — a real feel difference, not
	// just a bigger number on the card.
	S.ReloadSeconds /= FMath::Sqrt(Mult);
	return S;
}

float ComputeHitDamage(const FSFWeaponStats& Stats, float DistanceCm, bool bHeadshot)
{
	if (DistanceCm > Stats.MaxRange)
	{
		return 0.f;
	}

	const float Distance = FMath::Max(DistanceCm, 0.f);
	const float MinFrac = FMath::Clamp(Stats.MinDamageFraction, 0.f, 1.f);

	float Fraction = 1.f;
	if (Distance > Stats.FalloffStart)
	{
		const float Range = Stats.FalloffEnd - Stats.FalloffStart;
		if (Range <= KINDA_SMALL_NUMBER)
		{
			Fraction = MinFrac;
		}
		else
		{
			const float Alpha = FMath::Clamp((Distance - Stats.FalloffStart) / Range, 0.f, 1.f);
			Fraction = FMath::Lerp(1.f, MinFrac, Alpha);
		}
	}

	float Damage = Stats.BaseDamage * Fraction;
	if (bHeadshot)
	{
		Damage *= FMath::Max(Stats.HeadshotMultiplier, 1.f);
	}
	return FMath::Max(Damage, 0.f);
}

float SecondsPerShot(const FSFWeaponStats& Stats)
{
	return Stats.FireRate > KINDA_SMALL_NUMBER ? (1.f / Stats.FireRate) : 1.f;
}

float ApplyToShieldThenHealth(float Damage, float& InOutShield, float& InOutHealth)
{
	if (Damage <= 0.f)
	{
		return 0.f;
	}

	const float ToShield = FMath::Min(InOutShield, Damage);
	InOutShield -= ToShield;

	const float ToHealth = FMath::Min(InOutHealth, Damage - ToShield);
	InOutHealth -= ToHealth;
	return ToHealth;
}

} // namespace SFCombat
