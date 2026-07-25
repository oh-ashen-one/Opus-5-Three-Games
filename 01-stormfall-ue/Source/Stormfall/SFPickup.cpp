// Copyright Opus 5 Three Games. Original work.

#include "SFPickup.h"
#include "SFHealthComponent.h"
#include "SFWeaponComponent.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

ASFPickup::ASFPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetWorldScale3D(FVector(0.5f, 0.2f, 0.2f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CubeFinder.Object);
	}

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(RootComponent);
	PickupSphere->SetSphereRadius(110.f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ASFPickup::OnOverlap);
}

void ASFPickup::InitWeaponPickup(ESFWeaponClass Class, ESFRarity Rarity)
{
	Kind = ESFPickupKind::Weapon;
	Stats = SFCombat::MakeWeapon(Class, Rarity);

	// Rarer guns sit slightly larger, so the tier reads before you reach it.
	const float Scale = 0.45f + 0.06f * static_cast<float>(Rarity);
	Mesh->SetWorldScale3D(FVector(Scale * 2.f, Scale * 0.4f, Scale * 0.4f));
}

void ASFPickup::InitConsumable(ESFPickupKind InKind, float InAmount)
{
	Kind = InKind;
	Amount = InAmount;
	Mesh->SetWorldScale3D(FVector(0.35f, 0.35f, 0.5f));
}

bool ASFPickup::ApplyToActor(AActor* Target)
{
	if (!Target)
	{
		return false;
	}

	switch (Kind)
	{
		case ESFPickupKind::Weapon:
		{
			USFWeaponComponent* Weapon = Target->FindComponentByClass<USFWeaponComponent>();
			if (!Weapon)
			{
				return false;
			}
			Weapon->EquipWeapon(Stats);
			return true;
		}

		case ESFPickupKind::Shield:
		{
			USFHealthComponent* Health = Target->FindComponentByClass<USFHealthComponent>();
			if (!Health || Health->GetShield() >= Health->MaxShield)
			{
				// Refuse rather than silently consuming a full-shield player's potion.
				return false;
			}
			Health->AddShield(Amount);
			return true;
		}

		case ESFPickupKind::Heal:
		{
			USFHealthComponent* Health = Target->FindComponentByClass<USFHealthComponent>();
			if (!Health || Health->GetHealth() >= Health->MaxHealth)
			{
				return false;
			}
			Health->Heal(Amount);
			return true;
		}
	}
	return false;
}

void ASFPickup::OnOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (!Cast<APawn>(OtherActor))
	{
		return;
	}

	if (ApplyToActor(OtherActor))
	{
		Destroy();
	}
}
