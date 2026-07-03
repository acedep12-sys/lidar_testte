// STTask_TakeCover.cpp
#include "AI/StateTreeTasks/STTask_TakeCover.h"
#include "Actors/SquadAIAutoSetupManager.h"
#include "StateTreeExecutionContext.h"
#include "AI/SoldierAIController.h"
#include "Characters/SoldierCharacter.h"
#include "CoverSystem/CoverSystemSubsystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"
#include "Quest/QuestRegistry.h"
#include "SquadAITuning.h"

#define SQUADAI_VERBOSE_DEBUG_LOG(Format, ...) \
	do { if (ASquadAIAutoSetupManager::AreVerboseDebugLogsEnabled()) { UE_LOG(LogTemp, Warning, Format, ##__VA_ARGS__); } } while (false)

const UStruct* FSTTask_TakeCover::GetInstanceDataType() const { return FSTTask_TakeCoverData::StaticStruct(); }

static ASoldierAIController* GetTakeCoverController(FStateTreeExecutionContext& Context)
{
	if (ASoldierAIController* AIC = Cast<ASoldierAIController>(Context.GetOwner())) return AIC;
	if (APawn* Pawn = Cast<APawn>(Context.GetOwner())) return Cast<ASoldierAIController>(Pawn->GetController());
	return nullptr;
}

static FVector GetThreatLocation(ASoldierAIController* AI)
{
	float Confidence = 0.f;
	AActor* Threat = AI ? AI->GetPrimaryThreat(Confidence) : nullptr;
	if (Threat) return Threat->GetActorLocation();
	return (AI && AI->GetPawn())
		? AI->GetPawn()->GetActorLocation() + AI->GetPawn()->GetActorForwardVector() * 1000.f
		: FVector::ZeroVector;
}

static bool ProjectToNav(UWorld* World, const FVector& Candidate, FVector& OutProjected)
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys) return false;

	FNavLocation NavLoc;
	if (NavSys->ProjectPointToNavigation(Candidate, NavLoc, FVector(300.f, 300.f, 600.f)))
	{
		OutProjected = NavLoc.Location;
		return true;
	}
	return false;
}

static bool IsNaturallyCoveredFromThreat(ASoldierAIController* AI, const FVector& Candidate, const FVector& ThreatLoc)
{
	if (!AI || !AI->GetWorld()) return false;

	// Test both crouched torso and standing chest. If either is blocked by world geometry,
	// this point is useful natural/prop cover. This catches landscape ridges and sandbags
	// even when the procedural cover scanner has not generated a formal cover point.
	const FVector ThreatEye = ThreatLoc + FVector(0.f, 0.f, 90.f);
	const FVector TestHeights[] = {
		Candidate + FVector(0.f, 0.f, 65.f),
		Candidate + FVector(0.f, 0.f, 130.f)
	};

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TacticalCoverLOS), false);
	Params.AddIgnoredActor(AI->GetPawn());

	for (const FVector& TestPoint : TestHeights)
	{
		FHitResult Hit;
		if (AI->GetWorld()->LineTraceSingleByChannel(Hit, ThreatEye, TestPoint, ECC_Visibility, Params))
		{
			// Hitting the candidate pawn itself would not be cover. Hitting landscape/props is.
			if (Hit.GetActor() != AI->GetPawn())
			{
				return true;
			}
		}
	}

	return false;
}

static FVector GetMissionGoal(ASoldierAIController* AI)
{
	return AI ? AI->GetEffectiveMoveGoal() : FVector::ZeroVector;
}

static bool FindObstacleCoverBetweenThreatAndSelf(ASoldierAIController* AI, const FVector& ThreatLoc, FVector& OutGoal)
{
	if (!AI || !AI->GetPawn() || !AI->GetWorld()) return false;

	const FVector PawnLoc = AI->GetPawn()->GetActorLocation();
	const FVector ThreatEye = ThreatLoc + FVector(0.f, 0.f, 110.f);
	const FVector PawnChest = PawnLoc + FVector(0.f, 0.f, 90.f);
	const FVector ThreatToPawn = (PawnChest - ThreatEye).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ThreatToPawn).GetSafeNormal();

	const float SideOffsets[] = { 0.f, 120.f, -120.f, 260.f, -260.f, 420.f, -420.f };
	for (float Side : SideOffsets)
	{
		const FVector Start = ThreatEye + Right * Side;
		const FVector End = PawnChest + Right * Side;

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(FindObstacleCover), false);
		Params.AddIgnoredActor(AI->GetPawn());

		if (!AI->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			continue;
		}

		// Put the soldier on the protected side of the blocking object, close to it.
		const FVector AwayFromThreat = (Hit.ImpactPoint - ThreatLoc).GetSafeNormal2D();
		if (AwayFromThreat.IsNearlyZero()) continue;

		const float Standoffs[] = { 120.f, 180.f, 260.f };
		for (float Standoff : Standoffs)
		{
			FVector Candidate = Hit.ImpactPoint + AwayFromThreat * Standoff;
			FVector Projected;
			if (!ProjectToNav(AI->GetWorld(), Candidate, Projected)) continue;

			// Must actually provide at least crouch/chest protection.
			if (IsNaturallyCoveredFromThreat(AI, Projected, ThreatLoc))
			{
				OutGoal = Projected;
				return true;
			}
		}
	}

	return false;
}

static bool BuildFallbackCoverGoal(ASoldierAIController* AI, const FVector& ThreatLoc, float SearchRadius, FVector& OutGoal)
{
	if (!AI || !AI->GetPawn()) return false;

	UWorld* World = AI->GetWorld();
	const FVector PawnLoc = AI->GetPawn()->GetActorLocation();
	const FVector MissionGoal = GetMissionGoal(AI);
	const bool bHasGoal = !MissionGoal.IsNearlyZero();
	const float CurrentGoalDist = bHasGoal ? FVector::Dist(PawnLoc, MissionGoal) : 0.f;
	const float CurrentThreatDist = FVector::Dist(PawnLoc, ThreatLoc);

	// First priority: use actual geometry between threat and soldier (sandbags, rocks,
	// landscape lips). This prevents huge retreating moves and catches hand-placed cover.
	FVector InterceptGoal;
	if (FindObstacleCoverBetweenThreatAndSelf(AI, ThreatLoc, InterceptGoal))
	{
		if (!bHasGoal || FVector::Dist(InterceptGoal, MissionGoal) <= CurrentGoalDist + 350.f)
		{
			OutGoal = InterceptGoal;
			return true;
		}
	}

	const FVector ToGoal = bHasGoal ? (MissionGoal - PawnLoc).GetSafeNormal2D() : FVector::ZeroVector;
	const FVector AwayFromThreat = (PawnLoc - ThreatLoc).GetSafeNormal2D();
	const FVector ToThreat = (ThreatLoc - PawnLoc).GetSafeNormal2D();

	// Prefer small bounds toward the mission, with lateral offsets. Do NOT reward
	// retreating. If there is no mission goal, close slightly toward the enemy.
	FVector TacticalForward = bHasGoal ? ToGoal : ToThreat;
	if (TacticalForward.IsNearlyZero()) TacticalForward = AI->GetPawn()->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, TacticalForward).GetSafeNormal();

	float BestCoveredScore = -FLT_MAX;
	FVector BestCoveredGoal = FVector::ZeroVector;
	float BestAdvanceScore = -FLT_MAX;
	FVector BestAdvanceGoal = FVector::ZeroVector;

	const float ForwardSteps[] = { 250.f, 450.f, 650.f, 900.f };
	const float SideOffsets[] = { 0.f, 300.f, -300.f, 600.f, -600.f, 900.f, -900.f };

	for (float Forward : ForwardSteps)
	{
		for (float Side : SideOffsets)
		{
			const FVector Candidate = PawnLoc + TacticalForward * Forward + Right * Side;
			FVector Projected;
			if (!ProjectToNav(World, Candidate, Projected)) continue;

			const float TravelDist = FVector::Dist(Projected, PawnLoc);
			if (TravelDist > SearchRadius) continue;

			const float CandidateGoalDist = bHasGoal ? FVector::Dist(Projected, MissionGoal) : 0.f;
			const float CandidateThreatDist = FVector::Dist(Projected, ThreatLoc);
			const bool bIsCovered = IsNaturallyCoveredFromThreat(AI, Projected, ThreatLoc);

			// Never run far away from mission or kite far away from the fight.
			if (bHasGoal && CandidateGoalDist > CurrentGoalDist + 200.f) continue;
			if (CandidateThreatDist > CurrentThreatDist + 450.f) continue;

			if (bIsCovered)
			{
				float Score = 0.f;
				if (bHasGoal) Score += (CurrentGoalDist - CandidateGoalDist) * 2.5f;
				else Score += (CurrentThreatDist - CandidateThreatDist) * 1.2f;
				Score -= TravelDist * 0.10f;
				Score -= FMath::Abs(Side) * 0.03f;
				if (Score > BestCoveredScore)
				{
					BestCoveredScore = Score;
					BestCoveredGoal = Projected;
				}
			}
			else
			{
				// Open-ground bound: only a short advance, never a long retreat.
				if (TravelDist > 650.f) continue;
				if (bHasGoal && CandidateGoalDist >= CurrentGoalDist - 150.f) continue;
				float Score = (bHasGoal ? (CurrentGoalDist - CandidateGoalDist) : (CurrentThreatDist - CandidateThreatDist)) - TravelDist * 0.25f - FMath::Abs(Side) * 0.05f;
				if (Score > BestAdvanceScore)
				{
					BestAdvanceScore = Score;
					BestAdvanceGoal = Projected;
				}
			}
		}
	}

	if (BestCoveredScore > -FLT_MAX)
	{
		OutGoal = BestCoveredGoal;
		return true;
	}

	if (BestAdvanceScore > -FLT_MAX)
	{
		OutGoal = BestAdvanceGoal;
		return true;
	}

	// Emergency: small lateral step only. No endless backwards retreat.
	const float EmergencySides[] = { 300.f, -300.f, 550.f, -550.f };
	for (float Side : EmergencySides)
	{
		const FVector Candidate = PawnLoc + Right * Side;
		FVector Projected;
		if (!ProjectToNav(World, Candidate, Projected)) continue;
		if (bHasGoal && FVector::Dist(Projected, MissionGoal) > CurrentGoalDist + 100.f) continue;
		OutGoal = Projected;
		return true;
	}

	return false;
}

static void SetActiveCoverGoal(ASoldierAIController* AI, const FVector& Goal, bool bRealCover)
{
	if (!AI) return;
	AI->SetActiveCoverMoveGoal(Goal, bRealCover);
}

static EPathFollowingRequestResult::Type RequestCoverMove(ASoldierAIController* AI, const FVector& Goal, float AcceptanceRadius)
{
	if (!AI || !AI->GetPawn()) return EPathFollowingRequestResult::Failed;
	return AI->MoveToLocation(Goal, AcceptanceRadius, true, true, true, false, nullptr, true);
}

EStateTreeRunStatus FSTTask_TakeCover::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	auto& D = Context.GetInstanceData<FSTTask_TakeCoverData>(*this);
	D.Phase = FSTTask_TakeCoverData::EPhase::WaitingForResult;
	D.WaitTimer = 0.f;
	D.MoveGoal = FVector::ZeroVector;
	D.bHasMoveGoal = false;
	D.bUsingRealCover = false;
	D.LastMoveRequestTime = -1000.f;
	D.ReservedChunkKey = FIntPoint::ZeroValue;
	D.ReservedLocalID = -1;
	D.bReserved = false;

	ASoldierAIController* AI = GetTakeCoverController(Context);
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;

	const float Now = AI->GetWorld()->GetTimeSeconds();

	if ((Now - AI->GetLastTakeCoverCompletedTime()) < AI->TakeCoverReentryCooldown)
	{
		// Recently completed a cover/reposition attempt. Let Engage run instead of letting
		// a global HasVision->TakeCover transition starve combat forever.
		return EStateTreeRunStatus::Succeeded;
	}

	ASoldierCharacter* SoldierForPackCheck = Cast<ASoldierCharacter>(AI->GetPawn());
	float ThreatConfidenceForPackCheck = 0.f;
	AActor* ThreatForPackCheck = AI->GetPrimaryThreat(ThreatConfidenceForPackCheck);
	const float ThreatDistForPackCheck = ThreatForPackCheck ? FVector::Dist(AI->GetPawn()->GetActorLocation(), ThreatForPackCheck->GetActorLocation()) : FLT_MAX;
	const bool bPackCloseCombat = SoldierForPackCheck && SoldierForPackCheck->IsUsingPackPresentationMode() && ThreatForPackCheck && ThreatConfidenceForPackCheck >= 0.2f && ThreatDistForPackCheck <= 2200.f && AI->HasLineOfSightTo(ThreatForPackCheck);
	if (bPackCloseCombat)
	{
		// At point-blank / close range, running away to cover looks wrong and can prevent fighting.
		// Let Engage/direct combat handle the fight. Mark a cover attempt completed to avoid re-entry spam.
		AI->bHasCover = false;
		AI->ClearActiveCoverMoveGoal(true);
		SoldierForPackCheck->ClearCover();
		SQUADAI_VERBOSE_DEBUG_LOG(TEXT("TakeCover: %s skipped close pack combat. Threat=%s Dist=%.0f"), *GetNameSafe(AI->GetPawn()), *GetNameSafe(ThreatForPackCheck), ThreatDistForPackCheck);
		return EStateTreeRunStatus::Succeeded;
	}

	// Critical fix: StateTree global transitions can re-enter this task every tick while
	// a target is visible. Resume the controller-level cover move instead of restarting
	// the cover query, otherwise the pawn never reaches the MoveTo part.
	if (AI->HasActiveCoverMoveGoal())
	{
		D.MoveGoal = AI->GetActiveCoverMoveGoal();
		D.bHasMoveGoal = true;
		D.bUsingRealCover = AI->IsActiveCoverMoveUsingRealCover();
		D.Phase = FSTTask_TakeCoverData::EPhase::Moving;
		return EStateTreeRunStatus::Running;
	}

	// Do not enqueue multiple async cover queries if re-entered rapidly.
	if ((Now - AI->GetLastCoverRequestTime()) < 0.75f && !AI->bCoverQueryFinished)
	{
		return EStateTreeRunStatus::Running;
	}

	const float EffectiveSearchRadius = FMath::Max(SearchRadius, USquadAITuning::Get()->TacticalCoverSearchRadius);
	const FVector ThreatLoc = GetThreatLocation(AI);
	AI->RequestCover(ThreatLoc, EffectiveSearchRadius);

	UE_LOG(LogTemp, Verbose, TEXT("TakeCover: %s requested cover. Threat=%s Search=%.0f"),
		*GetNameSafe(AI->GetPawn()), *ThreatLoc.ToString(), EffectiveSearchRadius);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_TakeCover::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	auto& D = Context.GetInstanceData<FSTTask_TakeCoverData>(*this);
	ASoldierAIController* AI = GetTakeCoverController(Context);
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;

	APawn* Pawn = AI->GetPawn();
	ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(Pawn);
	if (!Soldier) return EStateTreeRunStatus::Failed;

	D.WaitTimer += DeltaTime;
	UPathFollowingComponent* PFC = AI->GetPathFollowingComponent();
	const float Now = AI->GetWorld()->GetTimeSeconds();

	// If this task was re-entered and instance data was reset, recover from controller state.
	if (!D.bHasMoveGoal && AI->HasActiveCoverMoveGoal())
	{
		D.MoveGoal = AI->GetActiveCoverMoveGoal();
		D.bHasMoveGoal = true;
		D.bUsingRealCover = AI->IsActiveCoverMoveUsingRealCover();
		D.Phase = FSTTask_TakeCoverData::EPhase::Moving;
	}

	switch (D.Phase)
	{
	case FSTTask_TakeCoverData::EPhase::WaitingForResult:
	{
		if (AI->bCoverQueryFinished && AI->bLastCoverQuerySucceeded && AI->bHasPendingCover)
		{
			AI->bHasPendingCover = false;
			const FCoverPoint& Cover = AI->PendingCoverResult;
			UCoverSystemSubsystem* CoverSys = AI->GetWorld()->GetSubsystem<UCoverSystemSubsystem>();
			if (CoverSys && CoverSys->ReserveCover(Cover.GetChunkKey(), Cover.LocalID, Pawn))
			{
				D.ReservedChunkKey = Cover.GetChunkKey();
				D.ReservedLocalID = Cover.LocalID;
				D.bReserved = true;
			}

			Soldier->SetCurrentCover(Cover);
			AI->CurrentCover = Cover;
			// Do not mark bHasCover until the pawn actually reaches the cover point.
			// Marking it here created stale/fake cover if movement failed or StateTree transitioned early.
			AI->bHasCover = false;
			D.MoveGoal = Cover.Location;
			D.bHasMoveGoal = true;
			D.bUsingRealCover = true;
			D.Phase = FSTTask_TakeCoverData::EPhase::Moving;
			SetActiveCoverGoal(AI, D.MoveGoal, true);

			const EPathFollowingRequestResult::Type Result = RequestCoverMove(AI, D.MoveGoal, AcceptanceRadius);
			D.LastMoveRequestTime = Now;
			SQUADAI_VERBOSE_DEBUG_LOG(TEXT("TakeCover: %s moving to real cover %s result=%d"),
				*GetNameSafe(Pawn), *D.MoveGoal.ToString(), static_cast<int32>(Result));
			if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
			{
					Soldier->SetCoverState(true, AI->CurrentCover.Height == ECoverHeight::Low);
					AI->bHasCover = true;
					SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAITakeCoverDebug] %s AlreadyAtRealCover Low=%d Loc=%s"), *GetNameSafe(Soldier), AI->CurrentCover.Height == ECoverHeight::Low ? 1 : 0, *AI->CurrentCover.Location.ToString());
					AI->ClearActiveCoverMoveGoal(true);
				D.Phase = FSTTask_TakeCoverData::EPhase::Arrived;
				return EStateTreeRunStatus::Succeeded;
			}
		}
		else if ((AI->bCoverQueryFinished || D.WaitTimer > 2.5f) && bUseFallbackRepositionIfNoCover)
		{
			FVector FallbackGoal;
			const float EffectiveSearchRadius = FMath::Max(SearchRadius, USquadAITuning::Get()->TacticalCoverSearchRadius);
			if (BuildFallbackCoverGoal(AI, GetThreatLocation(AI), EffectiveSearchRadius, FallbackGoal))
			{
				D.MoveGoal = FallbackGoal;
				D.bHasMoveGoal = true;
				D.bUsingRealCover = false;
				D.Phase = FSTTask_TakeCoverData::EPhase::Moving;
				AI->bHasCover = false;
				Soldier->ClearCover();
				SetActiveCoverGoal(AI, D.MoveGoal, false);

				const EPathFollowingRequestResult::Type Result = RequestCoverMove(AI, D.MoveGoal, AcceptanceRadius);
				D.LastMoveRequestTime = Now;
				SQUADAI_VERBOSE_DEBUG_LOG(TEXT("TakeCover: %s no real cover; reposition MoveTo %s result=%d"),
					*GetNameSafe(Pawn), *D.MoveGoal.ToString(), static_cast<int32>(Result));
				if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
				{
					AI->ClearActiveCoverMoveGoal(true);
					D.Phase = FSTTask_TakeCoverData::EPhase::Arrived;
					return EStateTreeRunStatus::Succeeded;
				}
			}
			else if (D.WaitTimer > 1.5f)
			{
				AI->bHasCover = false;
				Soldier->ClearCover();
				AI->ClearActiveCoverMoveGoal(true);
				SQUADAI_VERBOSE_DEBUG_LOG(TEXT("TakeCover: %s found no cover/fallback; proceeding to open-ground engage"), *GetNameSafe(Pawn));
				return EStateTreeRunStatus::Succeeded;
			}
		}
		else if (D.WaitTimer > (bUseFallbackRepositionIfNoCover ? 3.5f : 1.5f))
		{
			AI->bHasCover = false;
			Soldier->ClearCover();
			AI->ClearActiveCoverMoveGoal(true);
			SQUADAI_VERBOSE_DEBUG_LOG(TEXT("TakeCover: %s cover query timed out; proceeding to open-ground engage"), *GetNameSafe(Pawn));
			return EStateTreeRunStatus::Succeeded;
		}
		break;
	}

	case FSTTask_TakeCoverData::EPhase::Moving:
	{
		if (!D.bHasMoveGoal) return EStateTreeRunStatus::Failed;

		const float Dist = FVector::Dist(Pawn->GetActorLocation(), D.MoveGoal);
		if (Dist <= AcceptanceRadius)
		{
			if (D.bUsingRealCover)
			{
				Soldier->SetCoverState(true, AI->CurrentCover.Height == ECoverHeight::Low);
				AI->bHasCover = true;
				SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAITakeCoverDebug] %s ArrivedRealCover Low=%d Loc=%s Dist=%.0f"), *GetNameSafe(Soldier), AI->CurrentCover.Height == ECoverHeight::Low ? 1 : 0, *AI->CurrentCover.Location.ToString(), Dist);
				Pawn->SetActorRotation(AI->CurrentCover.Normal.Rotation());
			}
			else
			{
				// Fallback reposition is not real cover, so do not force crouch/cover flags.
				AI->bHasCover = false;
				Soldier->ClearCover();
			}

			AI->ClearActiveCoverMoveGoal(true);
			D.Phase = FSTTask_TakeCoverData::EPhase::Arrived;
			return EStateTreeRunStatus::Succeeded;
		}

		const bool bMoving = PFC && PFC->GetStatus() == EPathFollowingStatus::Moving;
		if (!bMoving && (Now - D.LastMoveRequestTime) > 0.75f)
		{
			const EPathFollowingRequestResult::Type Result = RequestCoverMove(AI, D.MoveGoal, AcceptanceRadius);
			D.LastMoveRequestTime = Now;
			SQUADAI_VERBOSE_DEBUG_LOG(TEXT("TakeCover: %s MoveTo %s result=%d dist=%.0f"),
				*GetNameSafe(Pawn), *D.MoveGoal.ToString(), static_cast<int32>(Result), Dist);

			if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
			{
				if (D.bUsingRealCover)
				{
					Soldier->SetCoverState(true, AI->CurrentCover.Height == ECoverHeight::Low);
					AI->bHasCover = true;
				}
				else
				{
					AI->bHasCover = false;
					Soldier->ClearCover();
				}
				AI->ClearActiveCoverMoveGoal(true);
				return EStateTreeRunStatus::Succeeded;
			}
			if (Result == EPathFollowingRequestResult::Failed)
			{
				AI->bHasCover = false;
				Soldier->ClearCover();
				AI->ClearActiveCoverMoveGoal(true);
				return EStateTreeRunStatus::Succeeded;
			}
		}

		// If stuck for too long, let the StateTree proceed to Engage instead of deadlocking.
		if (AI->HasActiveCoverMoveGoal() && (Now - AI->GetActiveCoverMoveStartedTime()) > 8.f)
		{
			SQUADAI_VERBOSE_DEBUG_LOG(TEXT("TakeCover: %s timed out moving to cover; proceeding to engage"), *GetNameSafe(Pawn));
			AI->bHasCover = false;
			Soldier->ClearCover();
			AI->ClearActiveCoverMoveGoal(true);
			return EStateTreeRunStatus::Succeeded;
		}
		break;
	}

	case FSTTask_TakeCoverData::EPhase::Arrived:
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FSTTask_TakeCover::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	auto& D = Context.GetInstanceData<FSTTask_TakeCoverData>(*this);
	ASoldierAIController* AI = GetTakeCoverController(Context);

	if (D.bReserved && AI)
	{
		if (UCoverSystemSubsystem* CoverSys = AI->GetWorld()->GetSubsystem<UCoverSystemSubsystem>())
		{
			CoverSys->ReleaseCover(D.ReservedChunkKey, D.ReservedLocalID);
		}
		D.bReserved = false;
	}

	// Do not clear ActiveCoverMoveGoal here. Re-entering this task should resume the
	// same movement goal instead of restarting the query every frame.
	// Do not ClearCover here either; Engage uses the cover state for peek/fire behavior.
	if (AI && !AI->HasActiveCoverMoveGoal())
	{
		AI->ClearMoveCommand();
	}
}
