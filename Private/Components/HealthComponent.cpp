// HealthComponent.cpp — Damage chokepoint with invincibility
#include "Components/HealthComponent.h"
#include "SquadAITuning.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	const USquadAITuning* T = USquadAITuning::Get();
	if (MaxHealth <= 0.f) MaxHealth = T->DefaultMaxHealth;
	if (bInvincible && InvincibleMinHealth <= 0.f) InvincibleMinHealth = T->InvincibleMinHealth;

	CurrentHealth = MaxHealth;
}

float UHealthComponent::ApplyDamage(float DamageAmount, AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.f) return 0.f;

	CurrentHealth -= DamageAmount;

	// Invincible: clamp but still fire events
	if (bInvincible)
	{
		CurrentHealth = FMath::Max(CurrentHealth, InvincibleMinHealth);
	}

	UE_LOG(LogTemp, Verbose, TEXT("Health: %s took %.1f from %s, HP %.1f/%.1f%s"),
		*GetNameSafe(GetOwner()), DamageAmount, *GetNameSafe(DamageCauser), CurrentHealth, MaxHealth,
		bInvincible ? TEXT(" invincible") : TEXT(""));

	// Broadcast damage event (for flinch animations, hit reactions, HUD)
	OnDamaged.Broadcast(DamageAmount, DamageCauser, CurrentHealth);
	OnHealthChanged.Broadcast(GetHealthPercent());

	// Death check
	if (CurrentHealth <= 0.f && !bInvincible)
	{
		bIsDead = true;
		UE_LOG(LogTemp, Warning, TEXT("Health: %s died"), *GetNameSafe(GetOwner()));
		OnDeath.Broadcast(GetOwner());
	}

	return DamageAmount;
}

float UHealthComponent::ApplyExternalDamage(float DamageAmount, AActor* DamageCauser, AController* InstigatorController, TSubclassOf<UDamageType> DamageTypeClass)
{
	const float Applied = ApplyDamage(DamageAmount, DamageCauser);
	if (Applied > 0.f)
	{
		OnExternalDamageReceived.Broadcast(Applied, DamageCauser, InstigatorController, DamageTypeClass);
	}
	return Applied;
}

void UHealthComponent::Heal(float Amount)
{
	if (Amount <= 0.f) return;

	if (bIsDead)
	{
		bIsDead = false;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(GetHealthPercent());
}
