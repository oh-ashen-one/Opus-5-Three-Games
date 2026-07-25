// Copyright Opus 5 Three Games. Original work.

#include "SFBotController.h"

#include "SFBuildComponent.h"
#include "SFHealthComponent.h"
#include "SFPickup.h"
#include "SFStormActor.h"
#include "SFWeaponComponent.h"

#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ASFBotController::ASFBotController()
{
	PrimaryActorTick.bCanEverTick = true;
	bAttachToPawn = false;
}

void ASFBotController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Vary aim error and spread per bot so a squad doesn't behave identically.
	AimStream.Initialize(GetTypeHash(GetName()));
	TimeSinceDamaged = 999.f;
	LastHealth = -1.f;
}

ASFStormActor* ASFBotController::FindStorm() const
{
	if (IsValid(CachedStorm))
	{
		return CachedStorm;
	}
	if (UWorld* World = GetWorld())
	{
		// Deliberately not a for-loop: there is exactly one storm, and a loop that
		// returns on its first iteration is a compile error under -Werror.
		TActorIterator<ASFStormActor> It(World);
		if (It)
		{
			CachedStorm = *It;
			return CachedStorm;
		}
	}
	return nullptr;
}

APawn* ASFBotController::FindNearestVisibleEnemy(APawn* Bot, float& OutDistance) const
{
	OutDistance = TNumericLimits<float>::Max();
	APawn* Best = nullptr;

	UWorld* World = GetWorld();
	if (!World || !Bot)
	{
		return nullptr;
	}

	const FVector Eye = Bot->GetActorLocation() + FVector(0.f, 0.f, 60.f);

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Other = *It;
		if (!IsValid(Other) || Other == Bot)
		{
			continue;
		}

		// Dead pawns are not targets.
		if (const USFHealthComponent* Health = Other->FindComponentByClass<USFHealthComponent>())
		{
			if (!Health->IsAlive())
			{
				continue;
			}
		}

		const float Distance = FVector::Dist(Eye, Other->GetActorLocation());
		if (Distance > SightRange || Distance >= OutDistance)
		{
			continue;
		}

		// Line of sight: structures block vision, which is what makes building work
		// defensively against bots at all.
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SFBotSight), false, Bot);
		Params.AddIgnoredActor(Other);
		const bool bBlocked = World->LineTraceSingleByChannel(
			Hit, Eye, Other->GetActorLocation() + FVector(0.f, 0.f, 40.f), ECC_Visibility, Params);
		if (bBlocked)
		{
			continue;
		}

		OutDistance = Distance;
		Best = Other;
	}

	return Best;
}

AActor* ASFBotController::FindNearestLoot(APawn* Bot, float& OutDistance) const
{
	OutDistance = TNumericLimits<float>::Max();
	AActor* Best = nullptr;

	UWorld* World = GetWorld();
	if (!World || !Bot)
	{
		return nullptr;
	}

	for (TActorIterator<ASFPickup> It(World); It; ++It)
	{
		ASFPickup* Pickup = *It;
		if (!IsValid(Pickup))
		{
			continue;
		}
		const float Distance = FVector::Dist(Bot->GetActorLocation(), Pickup->GetActorLocation());
		if (Distance < OutDistance)
		{
			OutDistance = Distance;
			Best = Pickup;
		}
	}
	return Best;
}

FSFBotContext ASFBotController::BuildContext(APawn* Bot, APawn*& OutEnemy) const
{
	FSFBotContext C;
	if (!Bot)
	{
		return C;
	}

	if (const USFHealthComponent* Health = Bot->FindComponentByClass<USFHealthComponent>())
	{
		C.Health = Health->GetHealth();
		C.Shield = Health->GetShield();
	}

	if (const USFWeaponComponent* Weapon = Bot->FindComponentByClass<USFWeaponComponent>())
	{
		C.bHasWeapon = Weapon->HasWeapon();
		C.AmmoInMag = Weapon->GetAmmoInMag();
		C.bReloading = Weapon->IsReloading();
	}

	if (const USFBuildComponent* Build = Bot->FindComponentByClass<USFBuildComponent>())
	{
		C.BuildMaterials = Build->GetMaterial(Build->GetSelectedMaterial());
	}

	if (const ASFStormActor* Storm = FindStorm())
	{
		const FSFStormState State = Storm->GetStormState();
		const FVector Location = Bot->GetActorLocation();
		const FVector2D Flat(Location.X, Location.Y);
		C.DistanceInsideStorm = State.Radius - FVector2D::Distance(Flat, State.Center);
		C.TimeUntilShrink = State.TimeToNextEvent;
		C.DistanceToSafety = FVector::Dist(Location, Storm->GetSafeDestination(Location));
	}

	float EnemyDistance = 0.f;
	OutEnemy = FindNearestVisibleEnemy(Bot, EnemyDistance);
	C.bEnemyVisible = (OutEnemy != nullptr);
	C.DistanceToEnemy = C.bEnemyVisible ? EnemyDistance : 100000.f;

	float LootDistance = 0.f;
	C.bLootKnown = (FindNearestLoot(Bot, LootDistance) != nullptr);
	C.DistanceToLoot = LootDistance;

	C.TimeSinceDamaged = TimeSinceDamaged;
	return C;
}

void ASFBotController::Execute(ESFBotAction Action, APawn* Bot, APawn* Enemy, float DeltaSeconds)
{
	if (!Bot)
	{
		return;
	}

	USFWeaponComponent* Weapon = Bot->FindComponentByClass<USFWeaponComponent>();
	USFBuildComponent* Build = Bot->FindComponentByClass<USFBuildComponent>();

	switch (Action)
	{
		case ESFBotAction::RotateToSafety:
		{
			if (const ASFStormActor* Storm = FindStorm())
			{
				MoveToLocation(Storm->GetSafeDestination(Bot->GetActorLocation()), 200.f);
			}
			break;
		}

		case ESFBotAction::Loot:
		{
			float Distance = 0.f;
			if (AActor* Loot = FindNearestLoot(Bot, Distance))
			{
				MoveToActor(Loot, 60.f);
			}
			break;
		}

		case ESFBotAction::Engage:
		{
			if (!Enemy || !Weapon)
			{
				break;
			}

			// Face the target, with a deliberate error cone. Perfect aim is not
			// difficulty, it's just unfair.
			const FVector ToEnemy = (Enemy->GetActorLocation() - Bot->GetActorLocation()).GetSafeNormal();
			const FVector Aim = AimStream.VRandCone(ToEnemy, FMath::DegreesToRadians(AimErrorDegrees));
			SetControlRotation(Aim.Rotation());
			Bot->SetActorRotation(FRotator(0.f, Aim.Rotation().Yaw, 0.f));

			// Close the distance a little rather than standing still trading.
			if (FVector::Dist(Bot->GetActorLocation(), Enemy->GetActorLocation()) > 1200.f)
			{
				MoveToActor(Enemy, 900.f);
			}
			else
			{
				StopMovement();
			}

			Weapon->TryFire();
			break;
		}

		case ESFBotAction::PanicBuild:
		{
			if (Build && Enemy)
			{
				// Wall toward the threat, not wherever the bot happened to face.
				const FVector ToEnemy = (Enemy->GetActorLocation() - Bot->GetActorLocation()).GetSafeNormal();
				Bot->SetActorRotation(FRotator(0.f, ToEnemy.Rotation().Yaw, 0.f));
				Build->SelectPiece(ESFBuildPiece::Wall);
				Build->TryBuild();
			}
			StopMovement();
			break;
		}

		case ESFBotAction::TakeCover:
		{
			// No cover-point system yet: back away from the threat, which at least
			// breaks line of sight against terrain and structures.
			if (Enemy)
			{
				const FVector Away = (Bot->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal();
				MoveToLocation(Bot->GetActorLocation() + Away * 1500.f, 150.f);
			}
			break;
		}

		case ESFBotAction::Reload:
		{
			if (Weapon)
			{
				Weapon->StartReload();
			}
			break;
		}

		case ESFBotAction::Idle:
		default:
			break;
	}
}

void ASFBotController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APawn* Bot = GetPawn();
	if (!Bot)
	{
		return;
	}

	// Track "was I shot recently" by watching health fall, rather than needing
	// every damage source to notify the controller.
	if (const USFHealthComponent* Health = Bot->FindComponentByClass<USFHealthComponent>())
	{
		const float Current = Health->GetHealth() + Health->GetShield();
		if (LastHealth >= 0.f && Current < LastHealth - KINDA_SMALL_NUMBER)
		{
			TimeSinceDamaged = 0.f;
		}
		LastHealth = Current;

		if (!Health->IsAlive())
		{
			StopMovement();
			return;
		}
	}
	TimeSinceDamaged += DeltaSeconds;

	ThinkAccumulator += DeltaSeconds;
	if (ThinkAccumulator < ThinkInterval)
	{
		// Between decisions, keep shooting if that's the standing order — the
		// weapon's own cooldown governs the actual rate.
		if (CurrentAction == ESFBotAction::Engage)
		{
			APawn* Enemy = nullptr;
			float Ignored = 0.f;
			Enemy = FindNearestVisibleEnemy(Bot, Ignored);
			if (Enemy)
			{
				if (USFWeaponComponent* Weapon = Bot->FindComponentByClass<USFWeaponComponent>())
				{
					Weapon->TryFire();
				}
			}
		}
		return;
	}
	ThinkAccumulator = 0.f;

	APawn* Enemy = nullptr;
	const FSFBotContext Context = BuildContext(Bot, Enemy);
	CurrentAction = SFBotBrain::Decide(Context, Tuning);
	Execute(CurrentAction, Bot, Enemy, DeltaSeconds);
}

void ASFBotController::HandleOwnerDamaged(AActor* /*Victim*/, AActor* /*Killer*/)
{
	TimeSinceDamaged = 0.f;
}
