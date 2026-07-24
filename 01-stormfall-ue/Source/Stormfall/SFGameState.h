// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SFGameState.generated.h"

/** Where the match is in its lifecycle. */
UENUM(BlueprintType)
enum class ESFMatchPhase : uint8
{
	Deploy		UMETA(DisplayName = "Deploying"),
	Battle		UMETA(DisplayName = "Battle"),
	Victory		UMETA(DisplayName = "Victory Royale"),
	Defeat		UMETA(DisplayName = "Eliminated"),
};

/** One line of the kill feed. */
USTRUCT(BlueprintType)
struct FSFKillFeedEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	FString KillerName;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	FString VictimName;

	/** True when the storm did it rather than another player. */
	UPROPERTY(BlueprintReadOnly, Category = "Match")
	bool bStormKill = false;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	float TimeStamp = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFPhaseChanged, ESFMatchPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFKillFeedUpdated, const FSFKillFeedEntry&, Entry);

/**
 * Match-wide state the HUD reads: who's left, what phase we're in, and the
 * kill feed. Single source of truth so the player counter and the victory
 * condition can never disagree.
 */
UCLASS()
class STORMFALL_API ASFGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Stormfall|Match")
	FSFPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stormfall|Match")
	FSFKillFeedUpdated OnKillFeedUpdated;

	UFUNCTION(BlueprintPure, Category = "Stormfall|Match")
	int32 GetPlayersRemaining() const { return PlayersRemaining; }

	UFUNCTION(BlueprintPure, Category = "Stormfall|Match")
	ESFMatchPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "Stormfall|Match")
	int32 GetPlayerEliminations() const { return PlayerEliminations; }

	UFUNCTION(BlueprintPure, Category = "Stormfall|Match")
	TArray<FSFKillFeedEntry> GetKillFeed() const { return KillFeed; }

	void SetPlayersRemaining(int32 Count);
	void SetPhase(ESFMatchPhase NewPhase);
	void AddKill(const FString& Killer, const FString& Victim, bool bStormKill);
	void AddPlayerElimination() { ++PlayerEliminations; }

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Match")
	int32 PlayersRemaining = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Match")
	int32 PlayerEliminations = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Match")
	ESFMatchPhase Phase = ESFMatchPhase::Deploy;

	/** Most recent entries first; trimmed so the HUD never grows unbounded. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stormfall|Match")
	TArray<FSFKillFeedEntry> KillFeed;

	static constexpr int32 MaxKillFeedEntries = 6;
};
