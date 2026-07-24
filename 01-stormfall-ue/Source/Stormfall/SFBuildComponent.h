// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SFBuild.h"
#include "SFBuildComponent.generated.h"

class ASFStructure;

/**
 * Per-pawn building: what you're holding, what you're carrying, and placing it.
 * Lives on players and bots alike so a bot panic-walling uses the same code path
 * the player does.
 */
UCLASS(ClassGroup = (Stormfall), meta = (BlueprintSpawnableComponent))
class STORMFALL_API USFBuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFBuildComponent();

	/** Place the currently selected piece where the owner is facing. */
	UFUNCTION(BlueprintCallable, Category = "Stormfall|Build")
	bool TryBuild();

	/** Add harvested material, clamped to the carry cap. */
	UFUNCTION(BlueprintCallable, Category = "Stormfall|Build")
	void AddMaterial(ESFBuildMaterial InMaterial, int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Stormfall|Build")
	int32 GetMaterial(ESFBuildMaterial InMaterial) const;

	UFUNCTION(BlueprintCallable, Category = "Stormfall|Build")
	void SelectPiece(ESFBuildPiece InPiece) { SelectedPiece = InPiece; }

	UFUNCTION(BlueprintCallable, Category = "Stormfall|Build")
	void SelectMaterial(ESFBuildMaterial InMaterial) { SelectedMaterial = InMaterial; }

	UFUNCTION(BlueprintPure, Category = "Stormfall|Build")
	ESFBuildPiece GetSelectedPiece() const { return SelectedPiece; }

	UFUNCTION(BlueprintPure, Category = "Stormfall|Build")
	ESFBuildMaterial GetSelectedMaterial() const { return SelectedMaterial; }

	/** Cycle to the next piece type. Bound to the mouse wheel / a key. */
	UFUNCTION(BlueprintCallable, Category = "Stormfall|Build")
	void CyclePiece(int32 Delta);

	/** Most players carry a few hundred of each; this caps hoarding. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Build")
	int32 CarryCap = 500;

	/** Starting materials, so building is possible before any harvesting. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Build")
	int32 StartingWood = 100;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Build")
	ESFBuildPiece SelectedPiece = ESFBuildPiece::Wall;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Build")
	ESFBuildMaterial SelectedMaterial = ESFBuildMaterial::Wood;

private:
	/** Carried amount per material, indexed by the enum's underlying value. */
	UPROPERTY()
	TArray<int32> Carried;

	void EnsureCarriedSized();
};
