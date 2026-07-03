// STTask_LeaderPath.cpp
#include "AI/StateTreeTasks/STTask_LeaderPath.h"
#include "AI/SoldierAIController.h"
#include "Characters/LeaderCharacter.h"
#include "Characters/SoldierCharacter.h"
#include "Components/AimComponent.h"
#include "Quest/QuestRegistry.h"
#include "SquadAITuning.h"
#include "Tasks/AITask_MoveTo.h"
#include "StateTreeExecutionContext.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

const UStruct* FSTTask_LeaderPath::GetInstanceDataType() const { return FSTTask_LeaderPathData::StaticStruct(); }

static ASoldierAIController* GetLeaderPathController(FStateTreeExecutionContext& Context)
{
	if (ASoldierAIController* AIC = Cast<ASoldierAIController>(Context.GetOwner()))
	{
		return AIC;
	}

	if (APawn* Pawn = Cast<APawn>(Context.GetOwner()))
	{
		return Cast<ASoldierAIController>(Pawn->GetController());
	}

	return nullptr;
}

EStateTreeRunStatus FSTTask_LeaderPath::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const {
	auto& D = Context.GetInstanceData<FSTTask_LeaderPathData>(*this);
	D.MoveTask = nullptr;
	D.LastTargetLoc = FVector::ZeroVector;
	D.TimeSinceRepath = 0.5f;
	D.bCombatCoverRequested = false;
	D.bMovingToCombatCover = false;
	D.CombatCoverGoal = FVector::ZeroVector;
	D.LastCombatCoverRequestTime = -1000.f;

	// When returning from combat/cover to mission path, make sure the leader is not
	// left crouched by TakeCover/Engage. Otherwise the AnimBP correctly stays in its
	// crouch state because CharacterMovement is still crouched.
	if (ASoldierAIController* AI = GetLeaderPathController(Context))
	{
		if (!AI->HasCombatThreat(0.10f) && !AI->HasNearbyHostile(FMath::Min(AI->DirectCombatRange, 8500.f)))
		{
			if (ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(AI->GetPawn()))
			{
				Soldier->ClearCover();
			}
		}
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_LeaderPath::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const {
	auto& D = Context.GetInstanceData<FSTTask_LeaderPathData>(*this);
	ASoldierAIController* AI = GetLeaderPathController(Context);
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;

	// Hard safety: the leader must never keep mission-pathing into hostiles.
	// If the StateTree asset fails to leave this state, this task becomes a tactical
	// combat-move task: cancel mission path, request cover, move there, and let the
	// controller fallback shoot while moving to cover.
	const float StopForHostileRange = FMath::Min(AI->DirectCombatRange, 8500.f);
	if (AI->HasCombatThreat(0.10f) || AI->HasNearbyHostile(StopForHostileRange))
	{
		if (D.MoveTask) { D.MoveTask->ExternalCancel(); D.MoveTask = nullptr; }

		float ThreatConfidence = 0.f;
		AActor* Threat = AI->GetPrimaryThreat(ThreatConfidence);
		const FVector ThreatLoc = Threat ? Threat->GetActorLocation() : AI->GetPawn()->GetActorLocation() + AI->GetPawn()->GetActorForwardVector() * 1000.f;
		if (ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(AI->GetPawn()))
		{
			FSquadAIAimCommand AimCommand;
			AimCommand.Mode = ESquadAIAimMode::CombatTarget;
			AimCommand.AimLocation = ThreatLoc + FVector(0.f, 0.f, 70.f);
			AimCommand.TargetActor = Threat;
			AI->SetAimCommand(AimCommand);
		}

		const float Now = AI->GetWorld()->GetTimeSeconds();
		if (!D.bCombatCoverRequested && !AI->HasActiveCoverMoveGoal())
		{
			AI->RequestCover(ThreatLoc, FMath::Max(3500.f, USquadAITuning::Get()->TacticalCoverSearchRadius));
			D.bCombatCoverRequested = true;
			D.LastCombatCoverRequestTime = Now;
			AI->StopMovement();
			return EStateTreeRunStatus::Running;
		}

		if (AI->bCoverQueryFinished && AI->bLastCoverQuerySucceeded && AI->bHasPendingCover && !D.bMovingToCombatCover)
		{
			D.CombatCoverGoal = AI->PendingCoverResult.Location;
			D.bMovingToCombatCover = true;
			AI->SetActiveCoverMoveGoal(D.CombatCoverGoal, true);
			AI->MoveToLocation(D.CombatCoverGoal, 600.f, true, true, true, false, nullptr, true);
			UE_LOG(LogTemp, Warning, TEXT("LeaderPathCombat: %s moving to cover %s"), *GetNameSafe(AI->GetPawn()), *D.CombatCoverGoal.ToString());
			return EStateTreeRunStatus::Running;
		}

		if (D.bMovingToCombatCover || AI->HasActiveCoverMoveGoal())
		{
			const float Dist = FVector::Dist(AI->GetPawn()->GetActorLocation(), D.CombatCoverGoal);
			if (Dist <= 650.f)
			{
				AI->ClearActiveCoverMoveGoal(true);
				// Do not blindly mark bHasCover here. LeaderPath only moved to a location
				// returned by the cover query; it did not transfer the full FCoverPoint to the
				// soldier or set cover state. A stale true bHasCover later made Engage fake
				// cover/crouch in open ground. Let TakeCover own real cover state.
				AI->bHasCover = false;
				if (ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(AI->GetPawn()))
				{
					Soldier->ClearCover();
				}
				return EStateTreeRunStatus::Succeeded;
			}
			return EStateTreeRunStatus::Running;
		}

		// If no cover is found within a short time, hold position instead of rushing.
		if ((Now - D.LastCombatCoverRequestTime) > 2.5f)
		{
			AI->StopMovement();
			return EStateTreeRunStatus::Succeeded;
		}

		AI->StopMovement();
		return EStateTreeRunStatus::Running;
	}

	// No combat threat: resume normal mission movement standing, not crouched.
	if (ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(AI->GetPawn()))
	{
		if (Soldier->bIsInCover || Soldier->bIsCrouched)
		{
			Soldier->ClearCover();
		}
	}

	D.TimeSinceRepath += DeltaTime;
	FVector GoalLoc = AI->GetEffectiveMoveGoal();

	if (GoalLoc.IsNearlyZero()) return EStateTreeRunStatus::Running;

	UPathFollowingComponent* PFC = AI->GetPathFollowingComponent();
	const bool bSameGoal = FVector::DistSquared(GoalLoc, D.LastTargetLoc) < 10000.f;
	const bool bPathActuallyMoving = PFC && PFC->GetStatus() == EPathFollowingStatus::Moving;

	// If the leader was previously waiting for the player, PathFollowing can become Idle
	// while the old UAITask_MoveTo still exists. In that case we must repath instead of
	// returning forever, otherwise the leader gets stuck after the player catches up.
	if (D.MoveTask && !D.MoveTask->IsFinished() && bSameGoal && bPathActuallyMoving)
	{
		return EStateTreeRunStatus::Running;
	}

	if (D.MoveTask) { D.MoveTask->ExternalCancel(); D.MoveTask = nullptr; }

	D.MoveTask = UAITask_MoveTo::AIMoveTo(AI, GoalLoc, nullptr, 500.f, EAIOptionFlag::Default, EAIOptionFlag::Enable);
	if (D.MoveTask) D.MoveTask->ReadyForActivation();
	
	D.LastTargetLoc = GoalLoc;
	D.TimeSinceRepath = 0.f;
	return EStateTreeRunStatus::Running;
}

void FSTTask_LeaderPath::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const {
	auto& D = Context.GetInstanceData<FSTTask_LeaderPathData>(*this);
	if (D.MoveTask) { D.MoveTask->ExternalCancel(); D.MoveTask = nullptr; }
}
