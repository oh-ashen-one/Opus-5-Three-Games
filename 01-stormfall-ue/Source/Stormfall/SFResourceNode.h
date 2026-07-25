// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFBuild.h"
#include "SFResourceNode.generated.h"

class UStaticMeshComponent;

/**
 * A harvestable tree/rock/vehicle. Shooting or hitting it yields materials to
 * whoever damaged it, which is why harvesting reuses the weapon trace rather than
 * needing a separate pickaxe system.
 */
UCLASS()
class STORMFALL_API ASFResourceNode : public AActor
{
	GENERATED_BODY()

public:
	ASFResourceNode();

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Stormfall|Resources")
	void InitNode(ESFBuildMaterial InMaterial);

	UFUNCTION(BlueprintPure, Category = "Stormfall|Resources")
	ESFBuildMaterial GetMaterial() const { return Material; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Stormfall|Resources")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stormfall|Resources")
	ESFBuildMaterial Material = ESFBuildMaterial::Wood;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stormfall|Resources")
	float NodeHealth = 200.f;

	/** Damage needed to pay out one harvest tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stormfall|Resources")
	float DamagePerYield = 34.f;

private:
	float DamageAccumulator = 0.f;

	void GrantMaterials(AActor* ToActor, int32 Amount);
};
