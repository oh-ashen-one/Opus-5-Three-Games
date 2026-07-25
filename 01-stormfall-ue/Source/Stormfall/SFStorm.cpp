// Copyright Opus 5 Three Games. Original work.

#include "SFStorm.h"

namespace SFStorm
{

float TotalDuration(const TArray<FSFStormPhase>& Phases)
{
	float Total = 0.f;
	for (const FSFStormPhase& Phase : Phases)
	{
		Total += FMath::Max(Phase.HoldSeconds, 0.f) + FMath::Max(Phase.ShrinkSeconds, 0.f);
	}
	return Total;
}

FSFStormState Evaluate(
	const TArray<FSFStormPhase>& Phases,
	const FVector2D& InitialCenter,
	float InitialRadius,
	float ElapsedSeconds)
{
	FSFStormState State;
	State.Center = InitialCenter;
	State.Radius = FMath::Max(InitialRadius, 0.f);

	if (Phases.Num() == 0)
	{
		State.bFinished = true;
		return State;
	}

	float T = FMath::Max(ElapsedSeconds, 0.f);
	FVector2D FromCenter = InitialCenter;
	float FromRadius = State.Radius;

	for (int32 i = 0; i < Phases.Num(); ++i)
	{
		const FSFStormPhase& Phase = Phases[i];
		const float Hold = FMath::Max(Phase.HoldSeconds, 0.f);
		const float Shrink = FMath::Max(Phase.ShrinkSeconds, 0.f);
		const float ToRadius = FromRadius * FMath::Max(Phase.RadiusScale, 0.f);
		const FVector2D ToCenter = Phase.TargetCenter;

		State.PhaseIndex = i;
		State.DamagePerSecond = Phase.DamagePerSecond;

		if (T < Hold)
		{
			// Holding: circle is still where the previous phase left it.
			State.Center = FromCenter;
			State.Radius = FromRadius;
			State.bShrinking = false;
			State.TimeToNextEvent = Hold - T;
			return State;
		}
		T -= Hold;

		if (T < Shrink)
		{
			const float Alpha = (Shrink > KINDA_SMALL_NUMBER) ? (T / Shrink) : 1.f;
			State.Center = FMath::Lerp(FromCenter, ToCenter, Alpha);
			State.Radius = FMath::Lerp(FromRadius, ToRadius, Alpha);
			State.bShrinking = true;
			State.TimeToNextEvent = Shrink - T;
			return State;
		}
		T -= Shrink;

		FromCenter = ToCenter;
		FromRadius = ToRadius;
	}

	// Past the last phase: the final circle persists and keeps doing damage, so a
	// stalemate still resolves rather than running forever.
	State.PhaseIndex = Phases.Num() - 1;
	State.Center = FromCenter;
	State.Radius = FromRadius;
	State.DamagePerSecond = Phases.Last().DamagePerSecond;
	State.bShrinking = false;
	State.TimeToNextEvent = 0.f;
	State.bFinished = true;
	return State;
}

bool IsOutside(const FSFStormState& State, const FVector& WorldLocation)
{
	const FVector2D Flat(WorldLocation.X, WorldLocation.Y);
	return FVector2D::Distance(Flat, State.Center) > State.Radius;
}

TArray<FSFStormPhase> DefaultPhases(float MapRadius)
{
	// Five phases, ~7.5 minutes of storm on top of the drop. Damage escalates hard
	// at the end so late-game rotations are genuinely lethal rather than a jog.
	// Centers drift off-origin so the final circle isn't always the map middle.
	const float R = FMath::Max(MapRadius, 1.f);

	TArray<FSFStormPhase> Phases;
	Phases.Add({ 90.f, 60.f, 0.62f, FVector2D(0.10f, -0.08f) * R, 1.f });
	Phases.Add({ 60.f, 50.f, 0.60f, FVector2D(0.18f, -0.02f) * R, 2.f });
	Phases.Add({ 45.f, 40.f, 0.58f, FVector2D(0.22f, 0.06f) * R, 5.f });
	Phases.Add({ 30.f, 30.f, 0.55f, FVector2D(0.24f, 0.10f) * R, 8.f });
	Phases.Add({ 20.f, 25.f, 0.10f, FVector2D(0.25f, 0.11f) * R, 12.f });
	return Phases;
}

} // namespace SFStorm
