// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFCombat.h"
#include "SFPickup.generated.h"

class UStaticMeshComponent;
class USphereComponent;

/** What a pickup gives you. */
UENUM(BlueprintType)
enum class ESFPickupKind : uint8
{
	Weapon	UMETA(DisplayName = "Weapon"),
	Shield	UMETA(DisplayName = "Shield Potion"),
	Heal	UMETA(DisplayName = "Bandage"),
};

/** Ground loot. Walk over it to take it. */
UCLASS()
class STORMFALL_API ASFPickup : public AActor
{
	GENERATED_BODY()

public:
	ASFPickup();

	UFUNCTION(BlueprintCallable, Category = "Stormfall|Loot")
	void InitWeaponPickup(ESFWeaponClass Class, ESFRarity Rarity);

	UFUNCTION(BlueprintCallable, Category = "Stormfall|Loot")
	void InitConsumable(ESFPickupKind InKind, float InAmount);

	/** Apply this pickup to a pawn. Returns false if it couldn't be used. */
	UFUNCTION(BlueprintCallable, Category = "Stormfall|Loot")
	bool ApplyToActor(AActor* Target);

	UFUNCTION(BlueprintPure, Category = "Stormfall|Loot")
	ESFPickupKind GetKind() const { return Kind; }

	UFUNCTION(BlueprintPure, Category = "Stormfall|Loot")
	ESFRarity GetRarity() const { return Stats.Rarity; }

protected:
	UFUNCTION()
	void OnOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "Stormfall|Loot")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Stormfall|Loot")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Loot")
	ESFPickupKind Kind = ESFPickupKind::Weapon;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Loot")
	FSFWeaponStats Stats;

	/** Shield or health restored, for consumables. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Loot")
	float Amount = 0.f;
};
