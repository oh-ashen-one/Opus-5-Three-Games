// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "SFCombat.generated.h"

/** Loot rarity. Five tiers with real stat deltas, not just colour. */
UENUM(BlueprintType)
enum class ESFRarity : uint8
{
	Common		UMETA(DisplayName = "Common"),
	Uncommon	UMETA(DisplayName = "Uncommon"),
	Rare		UMETA(DisplayName = "Rare"),
	Epic		UMETA(DisplayName = "Epic"),
	Legendary	UMETA(DisplayName = "Legendary"),
};

/** Weapon archetypes. */
UENUM(BlueprintType)
enum class ESFWeaponClass : uint8
{
	Pistol		UMETA(DisplayName = "Pistol"),
	AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	SMG			UMETA(DisplayName = "SMG"),
	Shotgun		UMETA(DisplayName = "Shotgun"),
	Sniper		UMETA(DisplayName = "Sniper"),
};

/** Everything that defines how a weapon shoots. */
USTRUCT(BlueprintType)
struct FSFWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	ESFWeaponClass Class = ESFWeaponClass::AssaultRifle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	ESFRarity Rarity = ESFRarity::Common;

	/** Damage per bullet at point blank, before headshot multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float BaseDamage = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float HeadshotMultiplier = 2.f;

	/** Shots per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FireRate = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 MagazineSize = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float ReloadSeconds = 2.2f;

	/** Pellets per shot. Shotguns fire many, everything else fires one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 Pellets = 1;

	/** Cone half-angle in degrees at the hip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float SpreadDegrees = 1.5f;

	/** Full damage out to here, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FalloffStart = 3000.f;

	/** Minimum damage fraction beyond FalloffEnd. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FalloffEnd = 9000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float MinDamageFraction = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float MaxRange = 25000.f;
};

/**
 * Combat math, pure and World-free so the damage curve can be tested directly.
 * If a number decides how much health someone loses, it is computed here.
 */
namespace SFCombat
{
	/** Damage/durability multiplier for a rarity tier. */
	STORMFALL_API float RarityMultiplier(ESFRarity Rarity);

	/** Human-readable tier name, for the HUD and pickup labels. */
	STORMFALL_API FString RarityName(ESFRarity Rarity);

	/** Base stats for a weapon class before rarity is applied. */
	STORMFALL_API FSFWeaponStats MakeWeapon(ESFWeaponClass Class, ESFRarity Rarity);

	/**
	 * Damage for one bullet at a distance.
	 *
	 * @param bHeadshot Applies the weapon's headshot multiplier.
	 * @return Damage in health points, never negative.
	 */
	STORMFALL_API float ComputeHitDamage(const FSFWeaponStats& Stats, float DistanceCm, bool bHeadshot);

	/** Seconds between shots for a weapon's fire rate. */
	STORMFALL_API float SecondsPerShot(const FSFWeaponStats& Stats);

	/**
	 * Damage applied to shield first, then health, the way a BR normally works.
	 * Returns the leftover damage that reached health.
	 */
	STORMFALL_API float ApplyToShieldThenHealth(float Damage, float& InOutShield, float& InOutHealth);
}
