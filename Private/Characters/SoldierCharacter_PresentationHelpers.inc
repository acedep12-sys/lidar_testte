#include "AI/SoldierAIController.h"
#include "Components/AimComponent.h"
#include "Characters/SquadLeaderTypes.h"

bool ASoldierCharacter::IsCombatReadyForPresentation(float CombatThreatConfidence) const
{
	if (const ASoldierAIController* AIC = Cast<ASoldierAIController>(GetController()))
	{
		return AIC->HasCombatThreat(CombatThreatConfidence);
	}
	return false;
}

ESquadCombatPresentationState ASoldierCharacter::GetSimplePresentationState(float SpeedThreshold, float CombatThreatConfidence) const
{
	if (!IsAlive())
	{
		return ESquadCombatPresentationState::Dead;
	}
	if (IsFiringForPresentation())
	{
		return ESquadCombatPresentationState::Firing;
	}
	if (IsAimingForPresentation() || IsCombatReadyForPresentation(CombatThreatConfidence))
	{
		return ESquadCombatPresentationState::Aiming;
	}
	if (IsMovingForPresentation(SpeedThreshold))
	{
		return ESquadCombatPresentationState::Moving;
	}
	return ESquadCombatPresentationState::Idle;
}

bool ASoldierCharacter::IsAimingForPresentation() const
{
	return AimComp && AimComp->bHasTarget;
}

bool ASoldierCharacter::IsFiringForPresentation() const
{
	return bWantsToFire;
}

bool ASoldierCharacter::IsMovingForPresentation(float SpeedThreshold) const
{
	return GetVelocity().SizeSquared2D() > FMath::Square(SpeedThreshold);
}
