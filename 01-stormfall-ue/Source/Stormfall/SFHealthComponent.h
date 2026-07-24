// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SFHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFDiedSignature, AActor*, Victim, AActor*, Killer);

/**
 * Health and shield for anything that can die. Shared by the player and bots so
 * eliminations, the kill feed, and the player counter all see one source of truth.
 */
UCLASS(ClassGroup = (Stormfall), meta = (BlueprintSpawnableComponent))
class STORMFALL_API USFHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFHealthComponent();

	UPROPERTY(BlueprintAssignable, Category = "Stormfall|Health")
	FSFDiedSignature OnDied;

	UFUNCTION(BlueprintPure, Category = "Stormfall|Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Stormfall|Health")
	float GetShield() const { return Shield; }

	UFUNCTION(BlueprintPure, Category = "Stormfall|Health")
	bool IsAlive() const { return Health > 0.f; }

	/** Apply damage through shield then health. Returns damage that hit health. */
	UFUNCTION(BlueprintCallable, Category = "Stormfall|Health")
	float ApplyDamage(float Amount, AActor* Instigator);

	UFUNCTION(BlueprintCallable, Category = "Stormfall|Health")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Stormfall|Health")
	void AddShield(float Amount);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Health")
	float MaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Health")
	float MaxShield = 100.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Health")
	float Health = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Health")
	float Shield = 0.f;

private:
	bool bDead = false;
};
