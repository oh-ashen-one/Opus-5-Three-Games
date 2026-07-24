// Copyright Opus 5 Three Games. Original work.

#include "SFWeaponComponent.h"
#include "SFHealthComponent.h"
#include "SFStructure.h"

#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

USFWeaponComponent::USFWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USFWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// Seed from the owner's name so each pawn sprays differently but any single
	// pawn's spray is reproducible run to run.
	SpreadStream.Initialize(GetOwner() ? GetTypeHash(GetOwner()->GetName()) : 1337);

	if (!bHasWeapon)
	{
		EquipWeapon(SFCombat::MakeWeapon(ESFWeaponClass::AssaultRifle, ESFRarity::Common));
	}
}

void USFWeaponComponent::EquipWeapon(const FSFWeaponStats& InStats)
{
	Stats = InStats;
	AmmoInMag = InStats.MagazineSize;
	ReloadRemaining = 0.f;
	Cooldown = 0.f;
	bHasWeapon = true;
}

void USFWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Cooldown = FMath::Max(Cooldown - DeltaTime, 0.f);

	if (ReloadRemaining > 0.f)
	{
		ReloadRemaining -= DeltaTime;
		if (ReloadRemaining <= 0.f)
		{
			ReloadRemaining = 0.f;
			AmmoInMag = Stats.MagazineSize;
		}
	}
}

void USFWeaponComponent::StartReload()
{
	if (!bHasWeapon || ReloadRemaining > 0.f || AmmoInMag >= Stats.MagazineSize)
	{
		return;
	}
	ReloadRemaining = FMath::Max(Stats.ReloadSeconds, 0.01f);
}

void USFWeaponComponent::GetViewPoint(FVector& OutLocation, FVector& OutDirection) const
{
	OutLocation = FVector::ZeroVector;
	OutDirection = FVector::ForwardVector;

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Players shoot from the camera so the crosshair is honest. Bots have no
	// camera, so they shoot from eye height along their facing.
	if (const UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>())
	{
		OutLocation = Camera->GetComponentLocation();
		OutDirection = Camera->GetForwardVector();
		return;
	}

	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		OutLocation = Pawn->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		OutDirection = Pawn->GetControlRotation().Vector();
		return;
	}

	OutLocation = Owner->GetActorLocation();
	OutDirection = Owner->GetActorForwardVector();
}

void USFWeaponComponent::FirePellet(const FVector& Origin, const FVector& Direction)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector End = Origin + Direction * Stats.MaxRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SFWeaponTrace), /*bTraceComplex=*/true);
	Params.AddIgnoredActor(GetOwner());
	Params.bReturnPhysicalMaterial = false;

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Origin, End, ECC_Visibility, Params))
	{
		return;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		return;
	}

	const float Distance = FVector::Dist(Origin, Hit.ImpactPoint);

	// Head hits are decided by height on the capsule rather than by bone, since
	// the pawns are capsules until the animated mesh lands.
	bool bHeadshot = false;
	if (const APawn* HitPawn = Cast<APawn>(HitActor))
	{
		const float RelativeZ = Hit.ImpactPoint.Z - HitPawn->GetActorLocation().Z;
		bHeadshot = RelativeZ > 55.f;
	}

	const float Damage = SFCombat::ComputeHitDamage(Stats, Distance, bHeadshot);
	if (Damage <= 0.f)
	{
		return;
	}

	// Structures take damage through the normal actor path; pawns go through the
	// health component so shield is applied before health.
	if (USFHealthComponent* Health = HitActor->FindComponentByClass<USFHealthComponent>())
	{
		Health->ApplyDamage(Damage, GetOwner());
	}
	else
	{
		UGameplayStatics::ApplyDamage(HitActor, Damage, nullptr, GetOwner(), nullptr);
	}
}

bool USFWeaponComponent::TryFire()
{
	if (!bHasWeapon || Cooldown > 0.f || ReloadRemaining > 0.f)
	{
		return false;
	}

	if (AmmoInMag <= 0)
	{
		StartReload();
		return false;
	}

	FVector Origin, Forward;
	GetViewPoint(Origin, Forward);

	const int32 Pellets = FMath::Max(Stats.Pellets, 1);
	for (int32 i = 0; i < Pellets; ++i)
	{
		const FVector Direction = SpreadStream.VRandCone(Forward, FMath::DegreesToRadians(Stats.SpreadDegrees));
		FirePellet(Origin, Direction);
	}

	--AmmoInMag;
	Cooldown = SFCombat::SecondsPerShot(Stats);

	if (AmmoInMag <= 0)
	{
		StartReload();
	}
	return true;
}
