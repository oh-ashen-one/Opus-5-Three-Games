// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "SFBotBrain.generated.h"

/** What a bot has decided to do this think-tick. */
UENUM(BlueprintType)
enum class ESFBotAction : uint8
{
	Idle			UMETA(DisplayName = "Idle"),
	Loot			UMETA(DisplayName = "Loot"),
	RotateToSafety	UMETA(DisplayName = "Rotate To Safety"),
	Engage			UMETA(DisplayName = "Engage"),
	TakeCover		UMETA(DisplayName = "Take Cover"),
	PanicBuild		UMETA(DisplayName = "Panic Build"),
	Reload			UMETA(DisplayName = "Reload"),
};

/** Everything the brain is allowed to look at. Deliberately plain data. */
USTRUCT(BlueprintType)
struct FSFBotContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	float Health = 100.f;

	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	float Shield = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	bool bHasWeapon = false;

	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	int32 AmmoInMag = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	bool bReloading = false;

	/** Distance to the storm edge; negative means already outside taking damage. */
	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	float DistanceInsideStorm = 10000.f;

	/** Seconds until the circle starts closing again. */
	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	float TimeUntilShrink = 60.f;

	/** How far the bot must travel to reach safety. */
	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	float DistanceToSafety = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	bool bEnemyVisible = false;

	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	float DistanceToEnemy = 100000.f;

	/** Seconds since this bot last took damage. Large means "not in a fight". */
	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	float TimeSinceDamaged = 999.f;

	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	int32 BuildMaterials = 0;

	/** Distance to the nearest known loot pickup, if any. */
	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	bool bLootKnown = false;

	UPROPERTY(BlueprintReadWrite, Category = "Bot")
	float DistanceToLoot = 100000.f;
};

/** Bot difficulty knobs, so "they must lose believably" is tunable. */
USTRUCT(BlueprintType)
struct FSFBotTuning
{
	GENERATED_BODY()

	/** Health below which the bot stops trading and starts protecting itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bot")
	float PanicHealth = 40.f;

	/** Effective engagement range. Beyond this, shooting is a waste. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bot")
	float EngageRange = 6000.f;

	/** Speed the bot assumes it can travel, cm/s, when budgeting rotations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bot")
	float AssumedSpeed = 600.f;

	/** Safety margin on rotation timing. 1.5 = leave 50% earlier than needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bot")
	float RotationSafetyFactor = 1.5f;

	/** Below this magazine count the bot reloads rather than keep firing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bot")
	int32 ReloadThreshold = 3;
};

/**
 * Bot decision-making as a pure function of context. Kept out of the controller
 * so the whole behaviour table can be unit tested — "does a bot caught outside
 * the storm with an enemy in view still rotate?" is a test, not a playtest.
 */
namespace SFBotBrain
{
	/** Choose what to do this tick. */
	STORMFALL_API ESFBotAction Decide(const FSFBotContext& Context, const FSFBotTuning& Tuning);

	/** True when the bot must leave now or risk dying to the closing circle. */
	STORMFALL_API bool MustRotateNow(const FSFBotContext& Context, const FSFBotTuning& Tuning);
}
