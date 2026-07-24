// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SFGameMode.generated.h"

/**
 * Match flow for a STORMFALL round. Owns nothing gameplay-critical yet beyond
 * choosing the default pawn; the storm and match phases land here next.
 */
UCLASS()
class STORMFALL_API ASFGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASFGameMode();
};
