// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "SFCharacter.h"
#include "SFBotCharacter.generated.h"

/**
 * An AI opponent. Same pawn as the player — same health, weapon, and build
 * components — so a bot cannot have stats the player doesn't. Only the
 * controller differs.
 */
UCLASS()
class STORMFALL_API ASFBotCharacter : public ASFCharacter
{
	GENERATED_BODY()

public:
	ASFBotCharacter();
};
