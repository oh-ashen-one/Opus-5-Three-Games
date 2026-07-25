// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "SFStorm.generated.h"

/**
 * One storm phase: hold at the current circle, then shrink to a new one.
 * Damage applies for the whole phase, not just while shrinking.
 */
USTRUCT(BlueprintType)
struct FSFStormPhase
{
	GENERATED_BODY()

	/** Seconds the circle sits still before it starts closing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm")
	float HoldSeconds = 60.f;

	/** Seconds spent closing to the new circle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm")
	float ShrinkSeconds = 45.f;

	/** New radius as a fraction of the previous phase's radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm")
	float RadiusScale = 0.6f;

	/** Where the circle closes to, in world XY. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm")
	FVector2D TargetCenter = FVector2D::ZeroVector;

	/** Health per second lost outside the circle during this phase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm")
	float DamagePerSecond = 1.f;
};

/** Sampled storm state at one instant. */
USTRUCT(BlueprintType)
struct FSFStormState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Storm")
	int32 PhaseIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Storm")
	FVector2D Center = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Storm")
	float Radius = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Storm")
	float DamagePerSecond = 0.f;

	/** True while the circle is actively closing. */
	UPROPERTY(BlueprintReadOnly, Category = "Storm")
	bool bShrinking = false;

	/** Seconds until the circle next starts or finishes closing. */
	UPROPERTY(BlueprintReadOnly, Category = "Storm")
	float TimeToNextEvent = 0.f;

	/** True once every phase has completed. */
	UPROPERTY(BlueprintReadOnly, Category = "Storm")
	bool bFinished = false;
};

/**
 * Storm evaluation, kept as a pure function of (phases, elapsed time) so the whole
 * match timeline is unit testable without spawning a World. The actor in
 * SFStormActor just samples this every tick and applies the damage.
 */
namespace SFStorm
{
	/** Total seconds for the full phase list. */
	STORMFALL_API float TotalDuration(const TArray<FSFStormPhase>& Phases);

	/**
	 * Sample the storm at a point in time.
	 *
	 * @param Phases         Ordered phase list. Empty yields a finished state.
	 * @param InitialCenter  Circle center before phase 0 closes.
	 * @param InitialRadius  Circle radius before phase 0 closes.
	 * @param ElapsedSeconds Time since the match started. Negative clamps to 0.
	 */
	STORMFALL_API FSFStormState Evaluate(
		const TArray<FSFStormPhase>& Phases,
		const FVector2D& InitialCenter,
		float InitialRadius,
		float ElapsedSeconds);

	/** True when a world-space location is outside the circle and taking damage. */
	STORMFALL_API bool IsOutside(const FSFStormState& State, const FVector& WorldLocation);

	/** The default five-phase STORMFALL timeline, tuned for an 8-10 minute match. */
	STORMFALL_API TArray<FSFStormPhase> DefaultPhases(float MapRadius);
}
