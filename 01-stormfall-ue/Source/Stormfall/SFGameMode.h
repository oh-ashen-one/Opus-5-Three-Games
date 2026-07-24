// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SFGameMode.generated.h"

class ASFStormActor;
class ASFBotCharacter;

/**
 * Match flow: deploy, spawn the field, run the storm, and resolve to Victory
 * Royale or elimination. Owns the only authoritative count of who is alive.
 */
UCLASS()
class STORMFALL_API ASFGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASFGameMode();

	virtual void StartPlay() override;

	/** Number of AI opponents to spawn alongside the player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Match")
	int32 BotCount = 15;

	/** Radius of the playable island, cm. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Match")
	float MapRadius = 50000.f;

	/** Bots spawn within this fraction of the map radius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stormfall|Match")
	float SpawnSpreadFraction = 0.8f;

	UFUNCTION(BlueprintCallable, Category = "Stormfall|Match")
	void RestartMatch();

protected:
	UFUNCTION()
	void HandlePawnDied(AActor* Victim, AActor* Killer);

	void SpawnBots();
	void SpawnStorm();
	void RegisterPawn(APawn* Pawn);
	void CheckVictoryConditions();

	UPROPERTY()
	TObjectPtr<ASFStormActor> Storm;

	/** Every pawn still alive, player included. */
	UPROPERTY()
	TArray<TObjectPtr<APawn>> LivePawns;

private:
	FRandomStream SpawnStream;
	bool bMatchResolved = false;
};
