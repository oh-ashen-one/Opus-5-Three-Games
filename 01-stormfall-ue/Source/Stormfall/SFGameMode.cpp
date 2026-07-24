// Copyright Opus 5 Three Games. Original work.

#include "SFGameMode.h"
#include "SFCharacter.h"

ASFGameMode::ASFGameMode()
{
	DefaultPawnClass = ASFCharacter::StaticClass();
	PrimaryActorTick.bCanEverTick = false;
}
