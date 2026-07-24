// Copyright Opus 5 Three Games. Original work.

#include "SFStormActor.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ASFStormActor::ASFStormActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASFStormActor::BeginPlay()
{
	Super::BeginPlay();

	if (!bRunning)
	{
		// Autostart so the storm still runs if the map is opened directly rather
		// than through the game mode's match flow.
		StartStorm(FVector2D::ZeroVector, MapRadius);
	}
}

void ASFStormActor::StartStorm(const FVector2D& InCenter, float InRadius)
{
	InitialCenter = InCenter;
	InitialRadius = FMath::Max(InRadius, 1.f);
	MapRadius = InitialRadius;
	Phases = SFStorm::DefaultPhases(InitialRadius);
	Elapsed = 0.f;
	DamageAccumulator = 0.f;
	bRunning = true;

	CurrentState = SFStorm::Evaluate(Phases, InitialCenter, InitialRadius, 0.f);
}

void ASFStormActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bRunning)
	{
		return;
	}

	Elapsed += DeltaSeconds;
	CurrentState = SFStorm::Evaluate(Phases, InitialCenter, InitialRadius, Elapsed);

	DamageAccumulator += DeltaSeconds;
	if (DamageAccumulator >= DamageInterval)
	{
		ApplyStormDamage();
		DamageAccumulator -= DamageInterval;
	}
}

void ASFStormActor::ApplyStormDamage()
{
	UWorld* World = GetWorld();
	if (!World || CurrentState.DamagePerSecond <= 0.f)
	{
		return;
	}

	const float Damage = CurrentState.DamagePerSecond * DamageInterval;

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (!IsValid(Pawn))
		{
			continue;
		}
		if (SFStorm::IsOutside(CurrentState, Pawn->GetActorLocation()))
		{
			UGameplayStatics::ApplyDamage(Pawn, Damage, nullptr, this, nullptr);
		}
	}
}

FVector ASFStormActor::GetSafeDestination(const FVector& From) const
{
	const FVector2D Flat(From.X, From.Y);
	const FVector2D ToCenter = CurrentState.Center - Flat;
	const float Distance = ToCenter.Size();

	// Already comfortably inside: stay put.
	const float Margin = CurrentState.Radius * 0.75f;
	if (Distance <= Margin)
	{
		return From;
	}

	// Head to a point well inside the circle rather than the exact edge, so the
	// next shrink doesn't immediately catch you out again.
	const FVector2D Direction = Distance > KINDA_SMALL_NUMBER ? (ToCenter / Distance) : FVector2D(1.f, 0.f);
	const FVector2D Target = CurrentState.Center - Direction * Margin;
	return FVector(Target.X, Target.Y, From.Z);
}
