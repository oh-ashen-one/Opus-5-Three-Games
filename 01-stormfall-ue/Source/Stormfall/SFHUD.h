// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SFGameState.h"
#include "SFHUD.generated.h"

class ASFStormActor;
class ASFGameState;

/** Which screen the HUD is showing. */
UENUM(BlueprintType)
enum class ESFHUDScreen : uint8
{
	MainMenu,
	Playing,
	Paused,
	Settings,
	Victory,
	Defeat,
};

/**
 * Canvas-drawn HUD and menus.
 *
 * Deliberately not UMG: widget blueprints are binary .uasset files, and this
 * project keeps everything in source. Menus are keyboard-driven (arrows + Enter)
 * rather than mouse-driven, which avoids hand-rolling hit testing on canvas and
 * reads cleanly on camera.
 */
UCLASS()
class STORMFALL_API ASFHUD : public AHUD
{
	GENERATED_BODY()

public:
	ASFHUD();

	virtual void DrawHUD() override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Stormfall|UI")
	void SetScreen(ESFHUDScreen NewScreen);

	UFUNCTION(BlueprintPure, Category = "Stormfall|UI")
	ESFHUDScreen GetScreen() const { return Screen; }

	/** Menu navigation, driven by the player controller. */
	void MenuUp();
	void MenuDown();
	void MenuAdjust(int32 Delta);
	void MenuConfirm();
	void TogglePause();

	// ── Settings, applied live ───────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category = "Stormfall|Settings")
	float MouseSensitivity = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "Stormfall|Settings")
	float MasterVolume = 0.8f;

	UPROPERTY(BlueprintReadWrite, Category = "Stormfall|Settings")
	bool bInvertY = false;

protected:
	/** Flip to the win/lose screen when the match resolves. */
	UFUNCTION()
	void HandlePhaseChangedInternal(ESFMatchPhase NewPhase);

	void DrawPlaying();
	void DrawMainMenu();
	void DrawPauseMenu();
	void DrawSettingsMenu();
	void DrawEndScreen(bool bVictory);

	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Colour);
	void DrawPanel(float X, float Y, float Width, float Height);
	void DrawCentredText(const FString& Text, float Y, float Scale, const FLinearColor& Colour);
	void DrawMenuItems(const TArray<FString>& Items, float StartY, float Scale);

	ASFStormActor* FindStorm() const;

	UPROPERTY()
	ESFHUDScreen Screen = ESFHUDScreen::Playing;

	int32 MenuIndex = 0;

private:
	UPROPERTY()
	mutable TObjectPtr<ASFStormActor> CachedStorm;

	UPROPERTY()
	TObjectPtr<UFont> HUDFont;
};
