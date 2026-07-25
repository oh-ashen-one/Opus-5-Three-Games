// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SFCombat.h"
#include "SFWeaponComponent.generated.h"

/**
 * Hitscan shooting: fire rate gating, spread, magazine, reload, and damage via
 * the tested combat math. Used by the player and by bots — a bot that shoots you
 * is running exactly this code, so its damage and falloff match yours.
 */
UCLASS(ClassGroup = (Stormfall), meta = (BlueprintSpawnableComponent))
class STORMFALL_API USFWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFWeaponComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Equip a weapon, filling the magazine. */
	UFUNCTION(BlueprintCallable, Category = "Stormfall|Weapon")
	void EquipWeapon(const FSFWeaponStats& InStats);

	/** Fire from the owner's view. Returns true if a shot actually went off. */
	UFUNCTION(BlueprintCallable, Category = "Stormfall|Weapon")
	bool TryFire();

	UFUNCTION(BlueprintCallable, Category = "Stormfall|Weapon")
	void StartReload();

	UFUNCTION(BlueprintPure, Category = "Stormfall|Weapon")
	int32 GetAmmoInMag() const { return AmmoInMag; }

	UFUNCTION(BlueprintPure, Category = "Stormfall|Weapon")
	bool IsReloading() const { return ReloadRemaining > 0.f; }

	UFUNCTION(BlueprintPure, Category = "Stormfall|Weapon")
	FSFWeaponStats GetStats() const { return Stats; }

	UFUNCTION(BlueprintPure, Category = "Stormfall|Weapon")
	bool HasWeapon() const { return bHasWeapon; }

	/** Seconds of cooldown left before the next shot is allowed. */
	UFUNCTION(BlueprintPure, Category = "Stormfall|Weapon")
	float GetCooldownRemaining() const { return Cooldown; }

protected:
	virtual void BeginPlay() override;

	/** Where shots originate and aim. Camera for players, muzzle-forward for bots. */
	void GetViewPoint(FVector& OutLocation, FVector& OutDirection) const;

	/** Trace one pellet and apply damage. */
	void FirePellet(const FVector& Origin, const FVector& Direction);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Weapon")
	FSFWeaponStats Stats;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Weapon")
	int32 AmmoInMag = 0;

private:
	float Cooldown = 0.f;
	float ReloadRemaining = 0.f;
	bool bHasWeapon = false;

	/** Deterministic per-component stream so spread is reproducible in tests. */
	FRandomStream SpreadStream;
};
