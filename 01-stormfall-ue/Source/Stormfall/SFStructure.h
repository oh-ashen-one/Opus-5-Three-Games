// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFBuild.h"
#include "SFStructure.generated.h"

class UStaticMeshComponent;

/**
 * One placed structure piece. Destructible, material-tiered health, and it
 * unregisters its slot on death so the cell can be rebuilt immediately.
 */
UCLASS()
class STORMFALL_API ASFStructure : public AActor
{
	GENERATED_BODY()

public:
	ASFStructure();

	/** Configure geometry and health for a slot. Call right after spawning. */
	void InitPiece(const FSFBuildSlot& InSlot, ESFBuildMaterial InMaterial);

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Stormfall|Build")
	float GetHealthFraction() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	const FSFBuildSlot& GetSlot() const { return Slot; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(VisibleAnywhere, Category = "Stormfall|Build")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Build")
	FSFBuildSlot Slot;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Build")
	ESFBuildMaterial Material = ESFBuildMaterial::Wood;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Build")
	float Health = 150.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Build")
	float MaxHealth = 150.f;
};
