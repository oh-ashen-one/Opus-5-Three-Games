// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SFCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class USFBuildComponent;
class USFHealthComponent;
class USFWeaponComponent;
class UInputMappingContext;
struct FInputActionValue;

/**
 * Third-person player character. Movement feel lives here and is tuned in a grey
 * box before any level work happens: sprint, crouch, jump, and fall damage first;
 * slide and mantle build on top of the same speed/state plumbing.
 */
UCLASS()
class STORMFALL_API ASFCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASFCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;

	/** Horizontal speed the character is actually moving at, for HUD and anim. */
	UFUNCTION(BlueprintPure, Category = "Stormfall|Movement")
	float GetGroundSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Stormfall|Movement")
	bool IsSprinting() const { return bWantsToSprint && GetGroundSpeed() > 1.f; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ── Input ────────────────────────────────────────────────────────────────
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void SprintStart(const FInputActionValue& Value);
	void SprintStop(const FInputActionValue& Value);
	void CrouchToggle(const FInputActionValue& Value);
	void BuildPressed(const FInputActionValue& Value);
	void CycleBuildPiece(const FInputActionValue& Value);
	void FirePressed(const FInputActionValue& Value);
	void ReloadPressed(const FInputActionValue& Value);

	// ── Components ───────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stormfall|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stormfall|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/**
	 * Builds the mapping context and actions in code rather than loading .uasset
	 * input assets. Keeps input config reviewable in the diff and keeps binaries
	 * out of the repo, at the cost of not being editable from the editor UI.
	 */
	void BuildDefaultInput();

	// ── Input (constructed by BuildDefaultInput) ─────────────────────────────
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stormfall|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stormfall|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stormfall|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stormfall|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stormfall|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stormfall|Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stormfall|Input")
	TObjectPtr<UInputAction> BuildAction;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stormfall|Input")
	TObjectPtr<UInputAction> CyclePieceAction;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stormfall|Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stormfall|Input")
	TObjectPtr<UInputAction> ReloadAction;

	/** Building lives in a component so bots can use the identical code path. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stormfall|Build")
	TObjectPtr<USFBuildComponent> BuildComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stormfall|Health")
	TObjectPtr<USFHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stormfall|Weapon")
	TObjectPtr<USFWeaponComponent> WeaponComponent;

	// ── Movement tuning ──────────────────────────────────────────────────────
	/** Default ground speed, cm/s. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Movement")
	float WalkSpeed = 460.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Movement")
	float SprintSpeed = 720.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Movement")
	float CrouchSpeed = 260.f;

	/** How fast max speed interpolates between states. Higher is snappier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Movement")
	float SpeedBlendRate = 9.f;

	// ── Fall damage ──────────────────────────────────────────────────────────
	/** Fall speed below which landing is free, cm/s. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Damage")
	float SafeFallSpeed = 1200.f;

	/** Fall speed at which landing is lethal from full health, cm/s. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Damage")
	float LethalFallSpeed = 2600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Damage")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Damage")
	float Health = 100.f;

private:
	bool bWantsToSprint = false;

	/** Target max walk speed for the current state, before blending. */
	float DesiredMaxSpeed() const;
};
