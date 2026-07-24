// Copyright Opus 5 Three Games. Original work.

#include "SFBotBrain.h"

namespace SFBotBrain
{

bool MustRotateNow(const FSFBotContext& Context, const FSFBotTuning& Tuning)
{
	// Already outside and taking damage: nothing matters more than leaving.
	if (Context.DistanceInsideStorm <= 0.f)
	{
		return true;
	}

	// Budget the trip with a safety factor. A bot that leaves exactly on time
	// dies to the first thing that blocks its path, which reads as stupid.
	const float Speed = FMath::Max(Tuning.AssumedSpeed, 1.f);
	const float TravelSeconds = Context.DistanceToSafety / Speed;
	return TravelSeconds * FMath::Max(Tuning.RotationSafetyFactor, 1.f) >= Context.TimeUntilShrink;
}

ESFBotAction Decide(const FSFBotContext& Context, const FSFBotTuning& Tuning)
{
	// 1. The storm beats everything. A bot that wins a fight and then dies to the
	//    circle it ignored is the single most obviously broken-looking behaviour.
	if (MustRotateNow(Context, Tuning))
	{
		// One exception: if it's already being shot at while rotating and is hurt,
		// throw up a wall first. Running in a straight line while low is suicide.
		if (Context.bEnemyVisible
			&& Context.Health <= Tuning.PanicHealth
			&& Context.TimeSinceDamaged < 2.f
			&& Context.BuildMaterials > 0)
		{
			return ESFBotAction::PanicBuild;
		}
		return ESFBotAction::RotateToSafety;
	}

	// 2. Being shot at while hurt: protect yourself before trading.
	if (Context.Health <= Tuning.PanicHealth && Context.TimeSinceDamaged < 2.f)
	{
		if (Context.BuildMaterials > 0)
		{
			return ESFBotAction::PanicBuild;
		}
		return ESFBotAction::TakeCover;
	}

	// 3. Fight, if there's someone in view and in range and the gun is loaded.
	if (Context.bEnemyVisible && Context.DistanceToEnemy <= Tuning.EngageRange)
	{
		if (!Context.bHasWeapon)
		{
			// Seen, in range, and unarmed — this is not a fight worth having.
			return Context.bLootKnown ? ESFBotAction::Loot : ESFBotAction::TakeCover;
		}
		if (Context.bReloading)
		{
			// Mid-reload with someone shooting: get behind something.
			return Context.BuildMaterials > 0 ? ESFBotAction::PanicBuild : ESFBotAction::TakeCover;
		}
		if (Context.AmmoInMag <= 0)
		{
			return ESFBotAction::Reload;
		}
		return ESFBotAction::Engage;
	}

	// 4. Nobody around: top up the magazine rather than getting caught empty.
	if (Context.bHasWeapon && !Context.bReloading && Context.AmmoInMag <= Tuning.ReloadThreshold)
	{
		return ESFBotAction::Reload;
	}

	// 5. Go get gear — unarmed bots prioritise this over anything else calm.
	if ((!Context.bHasWeapon || Context.Shield <= 0.f) && Context.bLootKnown)
	{
		return ESFBotAction::Loot;
	}

	// 6. Nothing pressing: drift toward the middle so the endgame actually
	//    converges instead of everyone idling on the edge.
	if (Context.DistanceToSafety > 0.f)
	{
		return ESFBotAction::RotateToSafety;
	}

	return ESFBotAction::Idle;
}

} // namespace SFBotBrain
