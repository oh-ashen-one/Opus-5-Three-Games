// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SFPlayerController.generated.h"

/**
 * Routes menu navigation to the HUD. Menu input is bound here rather than on the
 * pawn because menus must still work while the game is paused and after the
 * player's pawn is dead.
 */
UCLASS()
class STORMFALL_API ASFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASFPlayerController();

protected:
	virtual void SetupInputComponent() override;

	void OnPause();
	void OnMenuUp();
	void OnMenuDown();
	void OnMenuLeft();
	void OnMenuRight();
	void OnMenuConfirm();
};
