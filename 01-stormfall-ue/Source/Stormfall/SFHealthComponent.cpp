// Copyright Opus 5 Three Games. Original work.

#include "SFHealthComponent.h"
#include "SFCombat.h"

USFHealthComponent::USFHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
}

float USFHealthComponent::ApplyDamage(float Amount, AActor* DamageInstigator)
{
	if (bDead || Amount <= 0.f)
	{
		return 0.f;
	}

	const float ToHealth = SFCombat::ApplyToShieldThenHealth(Amount, Shield, Health);

	if (Health <= 0.f && !bDead)
	{
		// Latch, so a burst that lands several bullets in one frame doesn't
		// broadcast the elimination more than once and double-count the kill feed.
		bDead = true;
		OnDied.Broadcast(GetOwner(), DamageInstigator);
	}
	return ToHealth;
}

void USFHealthComponent::Heal(float Amount)
{
	if (bDead || Amount <= 0.f)
	{
		return;
	}
	Health = FMath::Min(Health + Amount, MaxHealth);
}

void USFHealthComponent::AddShield(float Amount)
{
	if (bDead || Amount <= 0.f)
	{
		return;
	}
	Shield = FMath::Min(Shield + Amount, MaxShield);
}
