// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "SFGameState.h"

/**
 * Match resolution as a pure rule. Extracted from the game mode so the single
 * most important question in the game — "did I actually win?" — is a unit test
 * rather than something only a full 10-minute playthrough can check.
 */
namespace SFMatch
{
	/**
	 * Decide the phase the match should now be in.
	 *
	 * @param bPlayerAlive Whether the human player is still alive.
	 * @param AliveCount   Total pawns still alive, player included.
	 * @param Current      Current phase; terminal phases are never left.
	 */
	inline ESFMatchPhase ResolvePhase(bool bPlayerAlive, int32 AliveCount, ESFMatchPhase Current)
	{
		// Terminal states latch. Without this, the storm killing the last bot
		// after you've already won would flip Victory to Defeat.
		if (Current == ESFMatchPhase::Victory || Current == ESFMatchPhase::Defeat)
		{
			return Current;
		}

		if (!bPlayerAlive)
		{
			return ESFMatchPhase::Defeat;
		}

		// Player alive and nobody else left standing.
		if (AliveCount <= 1)
		{
			return ESFMatchPhase::Victory;
		}

		return Current;
	}
}
