// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure damage math, deliberately free of any Actor or World dependency so it can
 * be unit tested headlessly. Anything here must stay a plain function of its
 * arguments — if it needs the world, it belongs on the actor instead.
 */
namespace SFDamage
{
	/**
	 * Fall damage as a linear ramp between a free-landing speed and a speed that
	 * is lethal from full health.
	 *
	 * @param ImpactSpeed   Absolute vertical speed at the moment of landing, cm/s.
	 * @param SafeSpeed     At or below this, landing costs nothing.
	 * @param LethalSpeed   At or above this, landing costs MaxHealth.
	 * @param MaxHealth     Full health, i.e. the damage dealt at LethalSpeed.
	 * @return Damage in health points, clamped to [0, MaxHealth].
	 */
	inline float ComputeFallDamage(float ImpactSpeed, float SafeSpeed, float LethalSpeed, float MaxHealth)
	{
		if (ImpactSpeed <= SafeSpeed)
		{
			return 0.f;
		}

		// Guard a degenerate or inverted window: treat anything past Safe as lethal.
		const float Range = LethalSpeed - SafeSpeed;
		if (Range <= KINDA_SMALL_NUMBER)
		{
			return MaxHealth;
		}

		const float Alpha = FMath::Clamp((ImpactSpeed - SafeSpeed) / Range, 0.f, 1.f);
		return Alpha * MaxHealth;
	}
}
