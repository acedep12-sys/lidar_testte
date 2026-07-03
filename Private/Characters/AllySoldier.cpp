// AllySoldier.cpp — Immortal ally with PowerLevel-driven stats
#include "Characters/AllySoldier.h"
#include "Characters/LeaderCharacter.h"
#include "Components/WeaponComponent.h"
#include "Components/HealthComponent.h"
#include "Components/AimComponent.h"
#include "AI/SoldierAIController.h"
#include "SquadAITuning.h"
#include "EngineUtils.h"

AAllySoldier::AAllySoldier()
{
	Faction = ESquadFaction::Ally;
	TeamId = FGenericTeamId(0);
	AccuracyMultiplier = 0.25f; // Will be overridden by ApplyPowerLevel
}

void AAllySoldier::BeginPlay()
{
	Super::BeginPlay();

	// Read default power level from tuning
	const USquadAITuning* T = USquadAITuning::Get();
	if (PowerLevel == 0.25f) // Only if not overridden in Blueprint
	{
		PowerLevel = T->AllyDefaultPowerLevel;
	}

	ApplyPowerLevel();
	UpdatePresentationState();
}

void AAllySoldier::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	PresentationUpdateAccumulator += DeltaTime;
	if (PresentationUpdateAccumulator >= GetPresentationUpdateInterval())
	{
		PresentationUpdateAccumulator = 0.f;
		UpdatePresentationState();
	}
}

void AAllySoldier::ApplyPowerLevel()
{
	const USquadAITuning* T = USquadAITuning::Get();

	// ---- Health ----
	if (HealthComp)
	{
		HealthComp->bInvincible = T->bAlliesInvincible;
		HealthComp->MaxHealth = 150.f;
	}

	// ---- Accuracy ----
	const float DerivedAccuracy = FMath::Lerp(0.05f, 0.85f, PowerLevel);
	const float FinalAccuracy = (AccuracyOverride >= 0.f) ? AccuracyOverride : DerivedAccuracy;

	AccuracyMultiplier = FinalAccuracy;
	if (WeaponComp)
	{
		WeaponComp->AccuracyMultiplier = FinalAccuracy;
	}

	// ---- Damage Multiplier ----
	const float DerivedDamage = FMath::Lerp(0.20f, 1.40f, PowerLevel);
	const float FinalDamage = (DamageMultiplierOverride >= 0.f) ? DamageMultiplierOverride : DerivedDamage;

	if (WeaponComp)
	{
		WeaponComp->DamageMultiplier = FinalDamage;
	}

	// ---- Fire Rate ----
	const float DerivedFireRate = FMath::Lerp(0.70f, 1.30f, PowerLevel);
	const float FinalFireRate = (FireRateMultiplierOverride >= 0.f) ? FireRateMultiplierOverride : DerivedFireRate;

	if (WeaponComp)
	{
		WeaponComp->FireRatePerSec = USquadAITuning::Get()->DefaultFireRate * FinalFireRate;
	}

	UE_LOG(LogSquadAI, Verbose, TEXT("Ally %s: PowerLevel=%.2f, Accuracy=%.2f, Damage=%.2f, FireRate=%.2f"),
		*GetName(), PowerLevel, FinalAccuracy, FinalDamage, FinalFireRate);
}

bool AAllySoldier::IsPresentationFollowingLeader() const
{
	return bPresentationHasLeader && !bPresentationCombatReady && !bPresentationAiming && !bPresentationFiring;
}

bool AAllySoldier::IsPresentationHoldingForCombat() const
{
	return bPresentationCombatReady || bPresentationAiming || bPresentationFiring;
}

FText AAllySoldier::GetPresentationDebugStateText() const
{
	if (!IsAlive())
	{
		return FText::FromString(TEXT("Dead"));
	}
	if (IsPresentationHoldingForCombat())
	{
		return FText::FromString(TEXT("Combat"));
	}
	if (bPresentationHasLeader)
	{
		return FText::FromString(TEXT("FollowingLeader"));
	}
	return FText::FromString(TEXT("Idle"));
}

static ALeaderCharacter* ResolveAllyLeader(const AAllySoldier* Ally)
{
	if (!Ally || !Ally->GetWorld())
	{
		return nullptr;
	}

	if (ALeaderCharacter* OverrideLeader = Cast<ALeaderCharacter>(Ally->LeaderOverride.Get()))
	{
		return OverrideLeader;
	}

	for (TActorIterator<ALeaderCharacter> It(Ally->GetWorld()); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

float AAllySoldier::GetPresentationUpdateInterval() const
{
	if (const ASoldierAIController* AIC = Cast<ASoldierAIController>(GetController()))
	{
		switch (AIC->CurrentLODBucket)
		{
		case 0:
		case 1:
			return 0.1f;
		case 2:
			return 0.2f;
		case 3:
			return 0.35f;
		case 4:
			return 0.5f;
		default:
			return 0.75f;
		}
	}

	return 0.2f;
}

void AAllySoldier::UpdatePresentationState()
{
	bPresentationFiring = bWantsToFire;
	bPresentationAiming = AimComp && AimComp->bHasTarget;
	bPresentationCombatReady = false;
	if (const ASoldierAIController* AIC = Cast<ASoldierAIController>(GetController()))
	{
		bPresentationCombatReady = AIC->HasCombatThreat(0.25f);
	}

	if (ALeaderCharacter* Leader = ResolveAllyLeader(this))
	{
		bPresentationHasLeader = true;
		PresentationLeaderLocation = Leader->GetActorLocation();
		PresentationLeaderDistance = FVector::Dist(GetActorLocation(), PresentationLeaderLocation);
	}
	else
	{
		bPresentationHasLeader = false;
		PresentationLeaderLocation = FVector::ZeroVector;
		PresentationLeaderDistance = 0.f;
	}

	if (!IsAlive())
	{
		PresentationState = ESquadCombatPresentationState::Dead;
	}
	else if (bPresentationFiring)
	{
		PresentationState = ESquadCombatPresentationState::Firing;
	}
	else if (bPresentationAiming || bPresentationCombatReady)
	{
		PresentationState = ESquadCombatPresentationState::Aiming;
	}
	else if (GetVelocity().SizeSquared2D() > FMath::Square(10.f))
	{
		PresentationState = ESquadCombatPresentationState::Moving;
	}
	else
	{
		PresentationState = ESquadCombatPresentationState::Idle;
	}
}
