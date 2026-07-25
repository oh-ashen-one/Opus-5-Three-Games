// Copyright Opus 5 Three Games. Original work.

#include "SFGameState.h"
#include "Engine/World.h"

void ASFGameState::SetPlayersRemaining(int32 Count)
{
	PlayersRemaining = FMath::Max(Count, 0);
}

void ASFGameState::SetPhase(ESFMatchPhase NewPhase)
{
	if (Phase == NewPhase)
	{
		return;
	}
	Phase = NewPhase;
	OnPhaseChanged.Broadcast(Phase);
}

void ASFGameState::AddKill(const FString& Killer, const FString& Victim, bool bStormKill)
{
	FSFKillFeedEntry Entry;
	Entry.KillerName = bStormKill ? TEXT("The Storm") : Killer;
	Entry.VictimName = Victim;
	Entry.bStormKill = bStormKill;
	Entry.TimeStamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	KillFeed.Insert(Entry, 0);
	if (KillFeed.Num() > MaxKillFeedEntries)
	{
		KillFeed.SetNum(MaxKillFeedEntries);
	}
	OnKillFeedUpdated.Broadcast(Entry);
}
