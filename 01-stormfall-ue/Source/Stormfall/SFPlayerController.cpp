// Copyright Opus 5 Three Games. Original work.

#include "SFPlayerController.h"
#include "SFHUD.h"

ASFPlayerController::ASFPlayerController()
{
	// Menus must run while paused, or Escape would trap the player in the pause
	// screen with no way to navigate out of it.
	bShouldPerformFullTickWhenPaused = true;
}

void ASFPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ASFPlayerController::OnPause).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &ASFPlayerController::OnMenuUp).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &ASFPlayerController::OnMenuDown).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &ASFPlayerController::OnMenuLeft).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &ASFPlayerController::OnMenuRight).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ASFPlayerController::OnMenuConfirm).bExecuteWhenPaused = true;
}

void ASFPlayerController::OnPause()      { if (ASFHUD* H = Cast<ASFHUD>(GetHUD())) { H->TogglePause(); } }
void ASFPlayerController::OnMenuUp()     { if (ASFHUD* H = Cast<ASFHUD>(GetHUD())) { H->MenuUp(); } }
void ASFPlayerController::OnMenuDown()   { if (ASFHUD* H = Cast<ASFHUD>(GetHUD())) { H->MenuDown(); } }
void ASFPlayerController::OnMenuLeft()   { if (ASFHUD* H = Cast<ASFHUD>(GetHUD())) { H->MenuAdjust(-1); } }
void ASFPlayerController::OnMenuRight()  { if (ASFHUD* H = Cast<ASFHUD>(GetHUD())) { H->MenuAdjust(1); } }
void ASFPlayerController::OnMenuConfirm(){ if (ASFHUD* H = Cast<ASFHUD>(GetHUD())) { H->MenuConfirm(); } }
