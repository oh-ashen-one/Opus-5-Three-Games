// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFStorm.h"
#include "SFStormActor.generated.h"

/**
 * Drives the storm in-world: samples the pure timeline every tick, applies damage
 * to anything outside the circle, and exposes the current state to the HUD and to
 * bot rotation logic.
 */
UCLASS()
class STORMFALL_API ASFStormActor : public AActor
{
	GENERATED_BODY()

public:
	ASFStormActor();

	virtual void Tick(float DeltaSeconds) override;

	/** Begin the timeline. Called by the game mode when the match starts. */
	UFUNCTION(BlueprintCallable, Category = "Stormfall|Storm")
	void StartStorm(const FVector2D& InCenter, float InRadius);

	UFUNCTION(BlueprintPure, Category = "Stormfall|Storm")
	FSFStormState GetStormState() const { return CurrentState; }

	/** Nearest safe point for a pawn to rotate toward. Used by bots and the HUD. */
	UFUNCTION(BlueprintPure, Category = "Stormfall|Storm")
	FVector GetSafeDestination(const FVector& From) const;

	UFUNCTION(BlueprintPure, Category = "Stormfall|Storm")
	float GetElapsedSeconds() const { return Elapsed; }

protected:
	virtual void BeginPlay() override;

	/** How often storm damage ticks, in seconds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Storm")
	float DamageInterval = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Storm")
	float MapRadius = 50000.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Storm")
	FSFStormState CurrentState;

private:
	UPROPERTY()
	TArray<FSFStormPhase> Phases;

	FVector2D InitialCenter = FVector2D::ZeroVector;
	float InitialRadius = 50000.f;
	float Elapsed = 0.f;
	float DamageAccumulator = 0.f;
	bool bRunning = false;

	void ApplyStormDamage();
};
