// Copyright Opus 5 Three Games. Original work.

#include "SFBotCharacter.h"
#include "SFBotController.h"

ASFBotCharacter::ASFBotCharacter()
{
	AIControllerClass = ASFBotController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}
