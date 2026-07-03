// LeaderCharacter.cpp — Squad leader with waypoints, player pacing
#include "Characters/LeaderCharacter.h"
#include "Components/HealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SquadAITuning.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Quest/QuestDirector.h"
#include "Quest/QuestMission.h"
#include "Quest/QuestObjective.h"
#include "Performance/SoldierRegistrySubsystem.h"
#include "AI/SoldierAIController.h"
#include "Components/AimComponent.h"

ALeaderCharacter::ALeaderCharacter()
{
	Faction = ESquadFaction::Ally;
	TeamId = FGenericTeamId(0);
	AccuracyMultiplier = 0.55f; 

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f; 
}

void ALeaderCharacter::BeginPlay()
{
	Super::BeginPlay();

	const USquadAITuning* T = USquadAITuning::Get();

	if (HealthComp)
	{
		HealthComp->bInvincible = T->bLeaderInvincible;
		HealthComp->MaxHealth = 180.f;
	}

	WaitForPlayerRadius = T->LeaderWaitForPlayerRadius;
	FullSpeedRadius = T->LeaderFullSpeedRadius;
	NormalSpeed = T->LeaderNormalSpeed;
	WaitingSpeed = T->LeaderWaitingSpeed;
	UpdatePresentationState();
}

void ALeaderCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdatePacing();
	UpdateWaypointState(DeltaTime);

	PresentationUpdateAccumulator += DeltaTime;
	if (PresentationUpdateAccumulator >= GetPresentationUpdateInterval())
	{
		PresentationUpdateAccumulator = 0.f;
		UpdatePresentationState();
	}
}

void ALeaderCharacter::UpdatePacing()
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (!CMC) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) 
	{
		CMC->MaxWalkSpeed = NormalSpeed;
		return;
	}

	const float PlayerDist = GetPlayerDistance();
	if (PlayerDist > WaitForPlayerRadius) CMC->MaxWalkSpeed = 0.f;
	else if (PlayerDist > FullSpeedRadius)
	{
		const float Alpha = FMath::Clamp((PlayerDist - FullSpeedRadius) / (WaitForPlayerRadius - FullSpeedRadius), 0.f, 1.f);
		CMC->MaxWalkSpeed = FMath::Lerp(NormalSpeed, WaitingSpeed, Alpha);
	}
	else CMC->MaxWalkSpeed = NormalSpeed;
}

void ALeaderCharacter::UpdateWaypointState(float DeltaTime)
{
	if (IsQuestComplete() || QuestWaypoints.Num() == 0) return;

	switch (WaypointState)
	{
	case ELeaderWaypointState::NotReached:
	case ELeaderWaypointState::Moving:
		if (HasReachedCurrentWaypoint())
		{
			WaypointState = ELeaderWaypointState::Arrived;
			LingerTimer = 0.f;
		}
		break;
	case ELeaderWaypointState::Arrived:
	{
		const FQuestWaypoint& WP = QuestWaypoints[CurrentWaypointIndex];
		if (WP.bRequireAreaClear) WaypointState = ELeaderWaypointState::WaitingForClear;
		else { LingerTimer += DeltaTime; if (LingerTimer >= WP.LingerSeconds) AdvanceToNextWaypoint(); }
		break;
	}
	case ELeaderWaypointState::WaitingForClear:
		if (IsCurrentAreaClear()) { const FQuestWaypoint& WP = QuestWaypoints[CurrentWaypointIndex]; LingerTimer += DeltaTime; if (LingerTimer >= WP.LingerSeconds) AdvanceToNextWaypoint(); }
		else LingerTimer = 0.f;
		break;
	}
}

void ALeaderCharacter::SetQuestWaypoints(const TArray<FQuestWaypoint>& Waypoints)
{
	QuestWaypoints = Waypoints;
	CurrentWaypointIndex = 0;
	WaypointState = QuestWaypoints.Num() > 0 ? ELeaderWaypointState::NotReached : ELeaderWaypointState::Completed;
	LingerTimer = 0.f;
	UpdatePresentationState();
}

void ALeaderCharacter::BeginQuest()
{
	if (QuestWaypoints.Num() > 0)
	{
		WaypointState = ELeaderWaypointState::Moving;
		LingerTimer = 0.f;
		UpdatePresentationState();
	}
}

bool ALeaderCharacter::AdvanceToNextWaypoint()
{
	CurrentWaypointIndex++;
	LingerTimer = 0.f;
	if (CurrentWaypointIndex >= QuestWaypoints.Num()) { WaypointState = ELeaderWaypointState::Completed; return false; }
	WaypointState = ELeaderWaypointState::Moving;
	return true;
}

bool ALeaderCharacter::HasReachedCurrentWaypoint() const
{
	if (CurrentWaypointIndex >= QuestWaypoints.Num()) return false;
	const FVector WPLoc = QuestWaypoints[CurrentWaypointIndex].Location;
	return FVector::Dist(GetActorLocation(), WPLoc) <= QuestWaypoints[CurrentWaypointIndex].AcceptanceRadius;
}

bool ALeaderCharacter::IsCurrentAreaClear() const
{
	if (CurrentWaypointIndex >= QuestWaypoints.Num()) return true;
	const FQuestWaypoint& WP = QuestWaypoints[CurrentWaypointIndex];
	USoldierRegistrySubsystem* Reg = GetWorld()->GetSubsystem<USoldierRegistrySubsystem>();
	if (!Reg) return true;
	return Reg->CountHostilesNear(WP.Location, WP.AreaClearRadius, GetGenericTeamId().GetId()) == 0;
}

FVector ALeaderCharacter::GetCurrentWaypointLocation() const
{
	if (CurrentWaypointIndex >= QuestWaypoints.Num()) return GetActorLocation();
	return QuestWaypoints[CurrentWaypointIndex].Location;
}

float ALeaderCharacter::GetPlayerDistance() const
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	return Player ? FVector::Dist(GetActorLocation(), Player->GetActorLocation()) : 0.f;
}

bool ALeaderCharacter::IsWaitingForPlayer() const
{
	return GetPlayerDistance() > WaitForPlayerRadius;
}

bool ALeaderCharacter::IsQuestComplete() const { return WaypointState == ELeaderWaypointState::Completed; }

bool ALeaderCharacter::HasPresentationMoveGoal() const
{
	return bPresentationHasQuest && !PresentationMoveGoal.IsNearlyZero();
}

bool ALeaderCharacter::HasPresentationWaypointIndex() const
{
	return PresentationWaypointIndex != INDEX_NONE && QuestWaypoints.IsValidIndex(PresentationWaypointIndex);
}

bool ALeaderCharacter::IsPresentationFollowingMissionPath() const
{
	return bPresentationHasQuest && !bPresentationWaitingForPlayer && !bPresentationReachedWaypoint && !bPresentationCombatReady && !bPresentationAiming && !bPresentationFiring;
}

bool ALeaderCharacter::IsPresentationHoldingForCombat() const
{
	return bPresentationCombatReady || bPresentationAiming || bPresentationFiring;
}

bool ALeaderCharacter::IsPresentationAtMissionEnd() const
{
	return QuestWaypoints.Num() > 0 && IsQuestComplete();
}

FText ALeaderCharacter::GetPresentationDebugStateText() const
{
	if (!IsAlive())
	{
		return FText::FromString(TEXT("Dead"));
	}
	if (IsPresentationHoldingForCombat())
	{
		return FText::FromString(TEXT("Combat"));
	}
	if (bPresentationWaitingForPlayer)
	{
		return FText::FromString(TEXT("WaitingForPlayer"));
	}
	if (QuestWaypoints.Num() > 0)
	{
		if (IsQuestComplete())
		{
			return FText::FromString(TEXT("QuestComplete"));
		}
		if (bPresentationReachedWaypoint)
		{
			return bPresentationAreaClear ? FText::FromString(TEXT("WaypointClear")) : FText::FromString(TEXT("WaypointContested"));
		}
		return FText::FromString(TEXT("FollowingWaypoint"));
	}
	return FText::FromString(TEXT("Idle"));
}

FLeaderQuestProgressSnapshot ALeaderCharacter::GetQuestProgressSnapshot() const
{
	FLeaderQuestProgressSnapshot Snapshot;
	Snapshot.bQuestActive = QuestWaypoints.Num() > 0 && WaypointState != ELeaderWaypointState::Completed;
	Snapshot.bReachedCurrentWaypoint = HasReachedCurrentWaypoint();
	Snapshot.bCurrentAreaClear = IsCurrentAreaClear();
	Snapshot.bQuestComplete = IsQuestComplete();
	Snapshot.CurrentWaypointIndex = CurrentWaypointIndex;
	Snapshot.CurrentWaypointLocation = GetCurrentWaypointLocation();
	return Snapshot;
}

float ALeaderCharacter::GetPresentationUpdateInterval() const
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

void ALeaderCharacter::UpdatePresentationState()
{
	bPresentationHasQuest = QuestWaypoints.Num() > 0 && !IsQuestComplete();
	PresentationMoveGoal = GetCurrentWaypointLocation();
	PresentationPlayerDistance = GetPlayerDistance();
	bPresentationWaitingForPlayer = IsWaitingForPlayer();
	bPresentationReachedWaypoint = HasReachedCurrentWaypoint();
	bPresentationAreaClear = IsCurrentAreaClear();
	PresentationWaypointIndex = CurrentWaypointIndex;
	bPresentationFiring = bWantsToFire;
	bPresentationAiming = AimComp && AimComp->bHasTarget;
	bPresentationCombatReady = false;
	if (const ASoldierAIController* AIC = Cast<ASoldierAIController>(GetController()))
	{
		bPresentationCombatReady = AIC->HasCombatThreat(0.25f);
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
