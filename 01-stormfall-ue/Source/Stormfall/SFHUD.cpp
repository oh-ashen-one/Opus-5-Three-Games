// Copyright Opus 5 Three Games. Original work.

#include "SFHUD.h"

#include "SFBuildComponent.h"
#include "SFGameMode.h"
#include "SFGameState.h"
#include "SFHealthComponent.h"
#include "SFStormActor.h"
#include "SFWeaponComponent.h"
#include "SFCombat.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FLinearColor PanelColour(0.02f, 0.03f, 0.06f, 0.72f);
	const FLinearColor Accent(0.98f, 0.76f, 0.22f, 1.f);
	const FLinearColor HealthColour(0.35f, 0.85f, 0.40f, 1.f);
	const FLinearColor ShieldColour(0.32f, 0.66f, 0.98f, 1.f);
	const FLinearColor Muted(0.72f, 0.75f, 0.80f, 1.f);
	const FLinearColor StormColour(0.72f, 0.38f, 0.92f, 1.f);

	FString FormatClock(float Seconds)
	{
		const int32 Total = FMath::Max(FMath::CeilToInt(Seconds), 0);
		return FString::Printf(TEXT("%d:%02d"), Total / 60, Total % 60);
	}
}

ASFHUD::ASFHUD()
{
	PrimaryActorTick.bCanEverTick = false;
	HUDFont = GEngine ? GEngine->GetLargeFont() : nullptr;
}

void ASFHUD::BeginPlay()
{
	Super::BeginPlay();

	// The menu map has no game state; anything else starts in play.
	Screen = GetWorld() && GetWorld()->GetName().Contains(TEXT("Menu"))
		? ESFHUDScreen::MainMenu
		: ESFHUDScreen::Playing;

	if (ASFGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASFGameState>() : nullptr)
	{
		GS->OnPhaseChanged.AddDynamic(this, &ASFHUD::HandlePhaseChangedInternal);
	}
}

void ASFHUD::HandlePhaseChangedInternal(ESFMatchPhase NewPhase)
{
	if (NewPhase == ESFMatchPhase::Victory)
	{
		SetScreen(ESFHUDScreen::Victory);
	}
	else if (NewPhase == ESFMatchPhase::Defeat)
	{
		SetScreen(ESFHUDScreen::Defeat);
	}
}

ASFStormActor* ASFHUD::FindStorm() const
{
	if (IsValid(CachedStorm))
	{
		return CachedStorm;
	}
	if (UWorld* World = GetWorld())
	{
		TActorIterator<ASFStormActor> It(World);
		if (It)
		{
			CachedStorm = *It;
		}
	}
	return CachedStorm;
}

void ASFHUD::SetScreen(ESFHUDScreen NewScreen)
{
	Screen = NewScreen;
	MenuIndex = 0;

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	const bool bMenu = (Screen != ESFHUDScreen::Playing);
	PC->SetShowMouseCursor(bMenu);
	PC->SetPause(Screen == ESFHUDScreen::Paused);

	if (bMenu)
	{
		PC->SetInputMode(FInputModeGameAndUI());
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

// ── Drawing helpers ─────────────────────────────────────────────────────────

void ASFHUD::DrawPanel(float X, float Y, float Width, float Height)
{
	DrawRect(PanelColour, X, Y, Width, Height);
}

void ASFHUD::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Colour)
{
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), X, Y, Width, Height);
	DrawRect(Colour, X, Y, Width * FMath::Clamp(Fraction, 0.f, 1.f), Height);
}

void ASFHUD::DrawCentredText(const FString& Text, float Y, float Scale, const FLinearColor& Colour)
{
	if (!Canvas)
	{
		return;
	}
	float W = 0.f, H = 0.f;
	GetTextSize(Text, W, H, HUDFont, Scale);
	DrawText(Text, Colour, (Canvas->ClipX - W) * 0.5f, Y, HUDFont, Scale);
}

void ASFHUD::DrawMenuItems(const TArray<FString>& Items, float StartY, float Scale)
{
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const bool bSelected = (i == MenuIndex);
		const FString Line = bSelected ? FString::Printf(TEXT("> %s <"), *Items[i]) : Items[i];
		DrawCentredText(Line, StartY + i * 44.f, Scale, bSelected ? Accent : Muted);
	}
}

// ── Screens ─────────────────────────────────────────────────────────────────

void ASFHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	switch (Screen)
	{
		case ESFHUDScreen::MainMenu: DrawMainMenu(); break;
		case ESFHUDScreen::Paused:   DrawPlaying(); DrawPauseMenu(); break;
		case ESFHUDScreen::Settings: DrawPlaying(); DrawSettingsMenu(); break;
		case ESFHUDScreen::Victory:  DrawPlaying(); DrawEndScreen(true); break;
		case ESFHUDScreen::Defeat:   DrawPlaying(); DrawEndScreen(false); break;
		case ESFHUDScreen::Playing:
		default:                     DrawPlaying(); break;
	}
}

void ASFHUD::DrawPlaying()
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;

	APawn* Pawn = GetOwningPawn();

	// Crosshair — a gap cross, so it never hides what you're shooting at.
	{
		const float CX = W * 0.5f, CY = H * 0.5f;
		const FLinearColor C(1.f, 1.f, 1.f, 0.85f);
		DrawRect(C, CX - 12.f, CY - 1.f, 8.f, 2.f);
		DrawRect(C, CX + 4.f,  CY - 1.f, 8.f, 2.f);
		DrawRect(C, CX - 1.f, CY - 12.f, 2.f, 8.f);
		DrawRect(C, CX - 1.f, CY + 4.f,  2.f, 8.f);
	}

	// Health and shield, bottom left.
	if (Pawn)
	{
		if (const USFHealthComponent* Health = Pawn->FindComponentByClass<USFHealthComponent>())
		{
			const float BarX = 40.f, BarW = 300.f, BarH = 16.f;
			const float BarY = H - 90.f;
			DrawBar(BarX, BarY, BarW, BarH, Health->GetShield() / FMath::Max(Health->MaxShield, 1.f), ShieldColour);
			DrawBar(BarX, BarY + 22.f, BarW, BarH, Health->GetHealth() / FMath::Max(Health->MaxHealth, 1.f), HealthColour);
			DrawText(FString::Printf(TEXT("%d"), FMath::RoundToInt(Health->GetHealth())),
				FLinearColor::White, BarX + BarW + 12.f, BarY + 18.f, HUDFont, 1.1f);
		}

		// Weapon and ammo, bottom right.
		if (const USFWeaponComponent* Weapon = Pawn->FindComponentByClass<USFWeaponComponent>())
		{
			const FSFWeaponStats Stats = Weapon->GetStats();
			const FString Line = Weapon->IsReloading()
				? TEXT("RELOADING")
				: FString::Printf(TEXT("%d / %d"), Weapon->GetAmmoInMag(), Stats.MagazineSize);
			DrawText(Line, FLinearColor::White, W - 220.f, H - 70.f, HUDFont, 1.4f);
			DrawText(SFCombat::RarityName(Stats.Rarity), Accent, W - 220.f, H - 96.f, HUDFont, 0.9f);
		}

		// Build materials and current piece.
		if (const USFBuildComponent* Build = Pawn->FindComponentByClass<USFBuildComponent>())
		{
			const TCHAR* PieceNames[] = { TEXT("WALL"), TEXT("RAMP"), TEXT("FLOOR"), TEXT("ROOF") };
			const int32 PieceIdx = FMath::Clamp(static_cast<int32>(Build->GetSelectedPiece()), 0, 3);
			DrawText(FString::Printf(TEXT("[F] %s   [Q] cycle"), PieceNames[PieceIdx]),
				Muted, 40.f, H - 130.f, HUDFont, 0.9f);
			DrawText(FString::Printf(TEXT("Wood %d   Stone %d   Metal %d"),
					Build->GetMaterial(ESFBuildMaterial::Wood),
					Build->GetMaterial(ESFBuildMaterial::Stone),
					Build->GetMaterial(ESFBuildMaterial::Metal)),
				Muted, 40.f, H - 156.f, HUDFont, 0.9f);
		}
	}

	// Players remaining and eliminations, top right.
	if (const ASFGameState* GS = GetWorld()->GetGameState<ASFGameState>())
	{
		DrawPanel(W - 260.f, 24.f, 220.f, 74.f);
		DrawText(FString::Printf(TEXT("ALIVE  %d"), GS->GetPlayersRemaining()),
			FLinearColor::White, W - 244.f, 34.f, HUDFont, 1.3f);
		DrawText(FString::Printf(TEXT("ELIMS  %d"), GS->GetPlayerEliminations()),
			Muted, W - 244.f, 66.f, HUDFont, 1.0f);

		// Kill feed under it.
		const TArray<FSFKillFeedEntry> Feed = GS->GetKillFeed();
		for (int32 i = 0; i < Feed.Num(); ++i)
		{
			const FString Line = FString::Printf(TEXT("%s  x  %s"), *Feed[i].KillerName, *Feed[i].VictimName);
			DrawText(Line, Feed[i].bStormKill ? StormColour : Muted, W - 244.f, 112.f + i * 22.f, HUDFont, 0.8f);
		}
	}

	// Storm timer, top centre.
	if (const ASFStormActor* Storm = FindStorm())
	{
		const FSFStormState State = Storm->GetStormState();
		const FString Label = State.bShrinking ? TEXT("STORM CLOSING") : TEXT("STORM MOVES IN");
		DrawPanel(W * 0.5f - 130.f, 24.f, 260.f, 58.f);
		DrawCentredText(Label, 32.f, 0.9f, StormColour);
		DrawCentredText(FormatClock(State.TimeToNextEvent), 52.f, 1.3f, FLinearColor::White);

		// Distance to safety, so the player knows when to rotate.
		if (Pawn)
		{
			const FVector Safe = Storm->GetSafeDestination(Pawn->GetActorLocation());
			const float Distance = FVector::Dist(Pawn->GetActorLocation(), Safe);
			if (Distance > 100.f)
			{
				DrawCentredText(FString::Printf(TEXT("%dm to safety"), FMath::RoundToInt(Distance / 100.f)),
					92.f, 0.85f, Muted);
			}
		}
	}
}

void ASFHUD::DrawMainMenu()
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	DrawRect(FLinearColor(0.01f, 0.02f, 0.05f, 1.f), 0.f, 0.f, W, H);

	DrawCentredText(TEXT("S T O R M F A L L"), H * 0.24f, 3.0f, Accent);
	DrawCentredText(TEXT("Opus 5 Three Games  -  project 1 of 3"), H * 0.24f + 60.f, 0.9f, Muted);

	DrawMenuItems({ TEXT("PLAY"), TEXT("SETTINGS"), TEXT("QUIT") }, H * 0.48f, 1.4f);
	DrawCentredText(TEXT("arrows to move   enter to select"), H * 0.80f, 0.85f, Muted);
}

void ASFHUD::DrawPauseMenu()
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.62f), 0.f, 0.f, W, H);

	DrawCentredText(TEXT("PAUSED"), H * 0.30f, 2.2f, Accent);
	DrawMenuItems({ TEXT("RESUME"), TEXT("SETTINGS"), TEXT("QUIT TO DESKTOP") }, H * 0.46f, 1.3f);
}

void ASFHUD::DrawSettingsMenu()
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.72f), 0.f, 0.f, W, H);

	DrawCentredText(TEXT("SETTINGS"), H * 0.26f, 2.0f, Accent);
	DrawMenuItems({
		FString::Printf(TEXT("MOUSE SENSITIVITY    %.2f"), MouseSensitivity),
		FString::Printf(TEXT("MASTER VOLUME        %d%%"), FMath::RoundToInt(MasterVolume * 100.f)),
		FString::Printf(TEXT("INVERT Y             %s"), bInvertY ? TEXT("ON") : TEXT("OFF")),
		TEXT("BACK"),
	}, H * 0.42f, 1.2f);

	DrawCentredText(TEXT("left/right to adjust   enter to confirm"), H * 0.78f, 0.85f, Muted);
}

void ASFHUD::DrawEndScreen(bool bVictory)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	DrawRect(bVictory ? FLinearColor(0.10f, 0.08f, 0.00f, 0.72f)
					  : FLinearColor(0.06f, 0.00f, 0.00f, 0.72f), 0.f, 0.f, W, H);

	DrawCentredText(bVictory ? TEXT("VICTORY ROYALE") : TEXT("ELIMINATED"),
		H * 0.30f, bVictory ? 3.0f : 2.4f, bVictory ? Accent : FLinearColor(0.92f, 0.35f, 0.35f, 1.f));

	if (const ASFGameState* GS = GetWorld()->GetGameState<ASFGameState>())
	{
		DrawCentredText(FString::Printf(TEXT("Eliminations: %d"), GS->GetPlayerEliminations()),
			H * 0.30f + 70.f, 1.2f, FLinearColor::White);
		DrawCentredText(FString::Printf(TEXT("Players remaining: %d"), GS->GetPlayersRemaining()),
			H * 0.30f + 100.f, 1.0f, Muted);
	}

	DrawMenuItems({ TEXT("PLAY AGAIN"), TEXT("QUIT TO DESKTOP") }, H * 0.56f, 1.3f);
}

// ── Menu input ──────────────────────────────────────────────────────────────

void ASFHUD::MenuUp()
{
	if (Screen == ESFHUDScreen::Playing) { return; }
	MenuIndex = FMath::Max(MenuIndex - 1, 0);
}

void ASFHUD::MenuDown()
{
	if (Screen == ESFHUDScreen::Playing) { return; }
	const int32 Count =
		(Screen == ESFHUDScreen::Settings) ? 4 :
		(Screen == ESFHUDScreen::Victory || Screen == ESFHUDScreen::Defeat) ? 2 : 3;
	MenuIndex = FMath::Min(MenuIndex + 1, Count - 1);
}

void ASFHUD::MenuAdjust(int32 Delta)
{
	if (Screen != ESFHUDScreen::Settings)
	{
		return;
	}

	switch (MenuIndex)
	{
		case 0: MouseSensitivity = FMath::Clamp(MouseSensitivity + Delta * 0.05f, 0.1f, 3.f); break;
		case 1: MasterVolume = FMath::Clamp(MasterVolume + Delta * 0.05f, 0.f, 1.f); break;
		case 2: bInvertY = !bInvertY; break;
		default: break;
	}
}

void ASFHUD::TogglePause()
{
	if (Screen == ESFHUDScreen::Playing)
	{
		SetScreen(ESFHUDScreen::Paused);
	}
	else if (Screen == ESFHUDScreen::Paused || Screen == ESFHUDScreen::Settings)
	{
		SetScreen(ESFHUDScreen::Playing);
	}
}

void ASFHUD::MenuConfirm()
{
	UWorld* World = GetWorld();
	APlayerController* PC = GetOwningPlayerController();

	switch (Screen)
	{
		case ESFHUDScreen::MainMenu:
			if (MenuIndex == 0)      { UGameplayStatics::OpenLevel(World, FName(TEXT("Lvl_Island"))); }
			else if (MenuIndex == 1) { SetScreen(ESFHUDScreen::Settings); }
			else                     { if (PC) { PC->ConsoleCommand(TEXT("quit")); } }
			break;

		case ESFHUDScreen::Paused:
			if (MenuIndex == 0)      { SetScreen(ESFHUDScreen::Playing); }
			else if (MenuIndex == 1) { SetScreen(ESFHUDScreen::Settings); }
			else                     { if (PC) { PC->ConsoleCommand(TEXT("quit")); } }
			break;

		case ESFHUDScreen::Settings:
			if (MenuIndex == 3)      { SetScreen(ESFHUDScreen::Paused); }
			break;

		case ESFHUDScreen::Victory:
		case ESFHUDScreen::Defeat:
			if (MenuIndex == 0)
			{
				if (ASFGameMode* GM = World ? World->GetAuthGameMode<ASFGameMode>() : nullptr)
				{
					GM->RestartMatch();
				}
			}
			else if (PC)
			{
				PC->ConsoleCommand(TEXT("quit"));
			}
			break;

		default:
			break;
	}
}
