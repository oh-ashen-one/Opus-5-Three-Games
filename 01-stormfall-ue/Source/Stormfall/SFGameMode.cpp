// Copyright Opus 5 Three Games. Original work.

#include "SFGameMode.h"

#include "SFBotCharacter.h"
#include "SFCharacter.h"
#include "SFGameState.h"
#include "SFHealthComponent.h"
#include "SFStormActor.h"
#include "SFMatch.h"
#include "SFHUD.h"
#include "SFPlayerController.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ASFGameMode::ASFGameMode()
{
	DefaultPawnClass = ASFCharacter::StaticClass();
	GameStateClass = ASFGameState::StaticClass();
	HUDClass = ASFHUD::StaticClass();
	PlayerControllerClass = ASFPlayerController::StaticClass();
	PrimaryActorTick.bCanEverTick = false;
}

void ASFGameMode::StartPlay()
{
	Super::StartPlay();

	SpawnStream.Initialize(20260724);
	bMatchResolved = false;
	LivePawns.Reset();

	SpawnStorm();
	SpawnBots();

	// The player's pawn already exists by now; register it alongside the bots so
	// the alive count includes them.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		RegisterPawn(PC->GetPawn());
	}

	if (ASFGameState* GS = GetGameState<ASFGameState>())
	{
		GS->SetPlayersRemaining(LivePawns.Num());
		GS->SetPhase(ESFMatchPhase::Battle);
	}
}

void ASFGameMode::SpawnStorm()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Reuse a storm already placed in the level rather than spawning a second one.
	TActorIterator<ASFStormActor> It(World);
	if (It)
	{
		Storm = *It;
	}
	else
	{
		Storm = World->SpawnActor<ASFStormActor>(ASFStormActor::StaticClass(),
			FVector::ZeroVector, FRotator::ZeroRotator);
	}

	if (Storm)
	{
		Storm->StartStorm(FVector2D::ZeroVector, MapRadius);
	}
}

void ASFGameMode::SpawnBots()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Spread = MapRadius * FMath::Clamp(SpawnSpreadFraction, 0.f, 1.f);

	for (int32 i = 0; i < BotCount; ++i)
	{
		// Spread bots around a ring rather than clustering them, so the early game
		// isn't decided by one accidental spawn pile-up.
		const float Angle = (2.f * PI) * (static_cast<float>(i) / FMath::Max(BotCount, 1));
		const float Jitter = SpawnStream.FRandRange(0.45f, 1.f);
		const FVector Location(
			FMath::Cos(Angle) * Spread * Jitter,
			FMath::Sin(Angle) * Spread * Jitter,
			500.f);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ASFBotCharacter* Bot = World->SpawnActor<ASFBotCharacter>(
			ASFBotCharacter::StaticClass(), Location, FRotator(0.f, SpawnStream.FRandRange(0.f, 360.f), 0.f), Params);

		if (Bot)
		{
#if WITH_EDITOR
			// Editor-only API; guarded so a packaged build still compiles.
			Bot->SetActorLabel(FString::Printf(TEXT("Bot_%02d"), i + 1));
#endif
			RegisterPawn(Bot);
		}
	}
}

void ASFGameMode::RegisterPawn(APawn* Pawn)
{
	if (!Pawn || LivePawns.Contains(Pawn))
	{
		return;
	}

	LivePawns.Add(Pawn);

	if (USFHealthComponent* Health = Pawn->FindComponentByClass<USFHealthComponent>())
	{
		Health->OnDied.AddDynamic(this, &ASFGameMode::HandlePawnDied);
	}
}

void ASFGameMode::HandlePawnDied(AActor* Victim, AActor* Killer)
{
	APawn* VictimPawn = Cast<APawn>(Victim);
	if (!VictimPawn)
	{
		return;
	}

	LivePawns.Remove(VictimPawn);

	ASFGameState* GS = GetGameState<ASFGameState>();
	if (!GS)
	{
		return;
	}

	// A null killer means the storm — nothing else applies damage without one.
	const bool bStormKill = (Killer == nullptr) || Killer->IsA<ASFStormActor>();
	const FString KillerName = Killer ? Killer->GetName() : FString(TEXT("The Storm"));
	GS->AddKill(KillerName, VictimPawn->GetName(), bStormKill);

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;

	if (!bStormKill && Killer == PlayerPawn)
	{
		GS->AddPlayerElimination();
	}

	GS->SetPlayersRemaining(LivePawns.Num());

	// Hand a dead bot's controller nothing to do.
	if (VictimPawn != PlayerPawn)
	{
		VictimPawn->DetachFromControllerPendingDestroy();
		VictimPawn->SetLifeSpan(3.f);
	}

	CheckVictoryConditions();
}

void ASFGameMode::CheckVictoryConditions()
{
	if (bMatchResolved)
	{
		return;
	}

	ASFGameState* GS = GetGameState<ASFGameState>();
	if (!GS)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	const bool bPlayerAlive = PlayerPawn && LivePawns.Contains(PlayerPawn);

	const ESFMatchPhase Resolved = SFMatch::ResolvePhase(bPlayerAlive, LivePawns.Num(), GS->GetPhase());
	if (Resolved != GS->GetPhase())
	{
		bMatchResolved = true;
		GS->SetPhase(Resolved);
	}
}

void ASFGameMode::RestartMatch()
{
	if (UWorld* World = GetWorld())
	{
		// Full level reload: simplest way to guarantee no stale structures, loot,
		// or bot state leaks into the next match.
		UGameplayStatics::OpenLevel(World, FName(*World->GetName()));
	}
}
