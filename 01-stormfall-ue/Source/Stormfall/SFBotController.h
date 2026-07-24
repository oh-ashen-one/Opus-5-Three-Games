// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SFBotBrain.h"
#include "SFBotController.generated.h"

class ASFStormActor;

/**
 * Executes whatever SFBotBrain decides. All judgement lives in the brain; this
 * class only turns an action into movement, aim, and trigger pulls.
 */
UCLASS()
class STORMFALL_API ASFBotController : public AAIController
{
	GENERATED_BODY()

public:
	ASFBotController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION(BlueprintPure, Category = "Stormfall|Bot")
	ESFBotAction GetCurrentAction() const { return CurrentAction; }

protected:
	/** How often the bot re-decides, in seconds. Not every frame — that's wasteful
	 *  and makes bots twitch between actions on frame-to-frame noise. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Bot")
	float ThinkInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Bot")
	FSFBotTuning Tuning;

	/** Sight range for spotting enemies. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Bot")
	float SightRange = 9000.f;

	/** Degrees of aim error, so bots are beatable rather than laser-accurate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Bot")
	float AimErrorDegrees = 4.f;

private:
	FSFBotContext BuildContext(APawn* Bot, APawn*& OutEnemy) const;
	void Execute(ESFBotAction Action, APawn* Bot, APawn* Enemy, float DeltaSeconds);

	ASFStormActor* FindStorm() const;
	APawn* FindNearestVisibleEnemy(APawn* Bot, float& OutDistance) const;
	AActor* FindNearestLoot(APawn* Bot, float& OutDistance) const;

	UFUNCTION()
	void HandleOwnerDamaged(AActor* Victim, AActor* Killer);

	float ThinkAccumulator = 0.f;
	float TimeSinceDamaged = 999.f;
	float LastHealth = -1.f;
	ESFBotAction CurrentAction = ESFBotAction::Idle;

	UPROPERTY()
	mutable TObjectPtr<ASFStormActor> CachedStorm;

	FRandomStream AimStream;
};
