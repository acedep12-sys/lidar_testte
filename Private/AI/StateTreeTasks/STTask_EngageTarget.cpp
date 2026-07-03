// STTask_EngageTarget.cpp — Suppression-gated peek/fire/cooldown
#include "AI/StateTreeTasks/STTask_EngageTarget.h"
#include "Actors/SquadAIAutoSetupManager.h"
#include "StateTreeExecutionContext.h"
#include "AI/SoldierAIController.h"
#include "Characters/SoldierCharacter.h"
#include "Characters/EnemySoldier.h"
#include "Components/WeaponComponent.h"
#include "Components/AimComponent.h"
#include "Components/SquadAIAdapterComponent.h"
#include "AI/SquadAICommandTypes.h"
#include "SquadAITuning.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NativeGameplayTags.h"

#define SQUADAI_VERBOSE_DEBUG_LOG(Format, ...) \
	do { if (ASquadAIAutoSetupManager::AreVerboseDebugLogsEnabled()) { UE_LOG(LogTemp, Warning, Format, ##__VA_ARGS__); } } while (false)

const UStruct* FSTTask_EngageTarget::GetInstanceDataType() const
{
	return FSTTask_EngageTargetData::StaticStruct();
}

EStateTreeRunStatus FSTTask_EngageTarget::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	auto& D = Context.GetInstanceData<FSTTask_EngageTargetData>(*this);
	D.Phase = EEngagePhase::Waiting;
	D.PhaseTimer = 0.f;
	D.CyclesCompleted = 0;
	D.BlockedShotCount = 0;
	D.LastShotCheckTime = 0.f;
	D.bStartedFireForCurrentPeek = false;
	D.CurrentPeekDuration = FMath::RandRange(PeekDurationMin, PeekDurationMax);
	D.CurrentCooldownDuration = FMath::RandRange(CooldownMin, CooldownMax);

	ASoldierAIController* AI = Cast<ASoldierAIController>(Context.GetOwner());
	if (AI && AI->GetPawn())
	{
		if (ASoldierCharacter* S = Cast<ASoldierCharacter>(AI->GetPawn()))
		{
			// Do NOT blindly crouch here. This task can run in open ground after TakeCover
			// fails or while the StateTree is transitioning. Forcing cover here made AI crouch
			// in the open and wait forever. Only preserve crouch if TakeCover actually placed
			// the soldier in a real cover state.
			if (AI->bHasCover && S->bIsInCover)
			{
				S->SetCoverState(true, S->IsCurrentCoverLow());
				D.Phase = EEngagePhase::Waiting;
				SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIEngageDebug] Enter %s: RealCover=1 StartPhase=Waiting Crouched=%d"), *GetNameSafe(S), S->bIsCrouched ? 1 : 0);
			}
			else
			{
				S->ClearCover();
				D.Phase = EEngagePhase::Peeking; // open-ground/direct engage; no cover wait cycle
				SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIEngageDebug] Enter %s: RealCover=0 StartPhase=Peeking/OpenGround"), *GetNameSafe(S));
			}
		}
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_EngageTarget::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	auto& D = Context.GetInstanceData<FSTTask_EngageTargetData>(*this);

	ASoldierAIController* AI = Cast<ASoldierAIController>(Context.GetOwner());
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;

	ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(AI->GetPawn());
	if (!Soldier) return EStateTreeRunStatus::Failed;

	// No threat / no clear shot? Succeed out so StateTree can reposition instead of
	// shooting red debug lines into hills or through blocked terrain.
	float Confidence = 0.f;
	AActor* Threat = AI->GetPrimaryThreat(Confidence);
	const float EffectiveMaxEngageRange = FMath::Max(MaxEngageRange, USquadAITuning::Get()->MaxEngageRange);
	const float ThreatDist = Threat ? FVector::Dist(AI->GetPawn()->GetActorLocation(), Threat->GetActorLocation()) : FLT_MAX;
	if (!Threat || Confidence < 0.2f || ThreatDist > EffectiveMaxEngageRange)
	{
		AI->StopCombatPresentation();
		return EStateTreeRunStatus::Succeeded;
	}

	// Important: do NOT require line-of-sight while in Waiting phase.
	// EnterState deliberately crouches the soldier behind cover. If we require LOS here,
	// the task exits/re-enters forever and the AI only crouches without ever peeking.
	const bool bHasCombatLOS = AI->HasLineOfSightTo(Threat) || ThreatDist <= USquadAITuning::Get()->CombatCloseRangeOverride;
	const bool bActuallyInCover = AI->bHasCover && Soldier->bIsInCover;
	SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIEngageTickDebug] Pawn=%s Phase=%d Threat=%s Conf=%.2f Dist=%.0f LOS=%d AIHasCover=%d SoldierInCover=%d Crouched=%d StartedFire=%d Timer=%.2f Supp=%.2f"),
		*GetNameSafe(Soldier),
		static_cast<int32>(D.Phase),
		*GetNameSafe(Threat),
		Confidence,
		ThreatDist,
		bHasCombatLOS ? 1 : 0,
		AI->bHasCover ? 1 : 0,
		Soldier->bIsInCover ? 1 : 0,
		Soldier->bIsCrouched ? 1 : 0,
		D.bStartedFireForCurrentPeek ? 1 : 0,
		D.PhaseTimer,
		AI->GetSuppression());
	if (D.Phase != EEngagePhase::Waiting && !bHasCombatLOS)
	{
		AI->StopCombatPresentation();
		if (bActuallyInCover)
		{
			Soldier->SetCoverState(true, Soldier->IsCurrentCoverLow());
		}
		else
		{
			Soldier->ClearCover();
		}
		return EStateTreeRunStatus::Succeeded;
	}

	// Get suppression threshold from enemy config
	float SupThreshold = 0.6f;
	if (AEnemySoldier* Enemy = Cast<AEnemySoldier>(AI->GetPawn()))
	{
		SupThreshold = Enemy->SuppressionThreshold;
	}

	D.PhaseTimer += DeltaTime;

	switch (D.Phase)
	{
	// ---- WAITING (behind cover) ----
	case EEngagePhase::Waiting:
	{
		// Suppression gate — refuse to peek while suppressed
		if (AI->GetSuppression() > SupThreshold)
		{
			D.PhaseTimer = 0.f; // Reset timer — stay hidden
			return EStateTreeRunStatus::Running;
		}

		// Wait before peeking (randomized cooldown duration)
		if (D.PhaseTimer >= D.CurrentCooldownDuration)
		{
			// Peek!
			D.Phase = EEngagePhase::Peeking;
			D.PhaseTimer = 0.f;

			if (bActuallyInCover)
			{
				Soldier->SetCoverState(true, false); // Stand/peek from actual cover
			}
			else
			{
				Soldier->ClearCover(); // open-ground engage; never fake cover/crouch
			}

			// Aim at threat and start the SquadAI/pack burst through the adapter/controller path.
			AI->SetCombatAimTarget(Threat->GetActorLocation() + FVector(0.f, 0.f, 70.f), ESquadAIAimMode::CombatTarget, Threat);
			D.bStartedFireForCurrentPeek = AI->TryStartBurstAtCurrentAim();
			SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIEngageDebug] %s Waiting->Peeking Threat=%s FireStarted=%d"), *GetNameSafe(Soldier), *GetNameSafe(Threat), D.bStartedFireForCurrentPeek ? 1 : 0);
		}
		break;
	}

	// ---- PEEKING (exposed, firing) ----
	case EEngagePhase::Peeking:
	{
		// Update aim to track moving threat and start the first burst if this Peeking phase
		// was entered directly from open ground instead of through Waiting.
		if (Threat)
		{
			AI->SetCombatAimTarget(Threat->GetActorLocation() + FVector(0.f, 0.f, 70.f), ESquadAIAimMode::CombatTarget, Threat);
			if (!D.bStartedFireForCurrentPeek)
			{
				D.bStartedFireForCurrentPeek = AI->TryStartBurstAtCurrentAim();
				SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIEngageDebug] %s Peeking/OpenGround start fire Threat=%s FireStarted=%d"), *GetNameSafe(Soldier), *GetNameSafe(Threat), D.bStartedFireForCurrentPeek ? 1 : 0);
			}
		}
		
		// Optional logic to detect if we're constantly shooting into a sandbag or landscape.
		// If so, increment a counter. If it reaches MaxBlockedShots, we force a Succeeded out so the AI finds new cover.
		if (AI->IsBurstFiring())
		{
			const float CurrentTime = Soldier->GetWorld()->GetTimeSeconds();
			if (CurrentTime - D.LastShotCheckTime > 0.5f) // Poll every 0.5s while firing
			{
				D.LastShotCheckTime = CurrentTime;
				
				FTransform MuzzleWorld;
				if (Soldier->GetMuzzleWorldTransform(MuzzleWorld)) 
				{
					FVector MuzzleLoc = MuzzleWorld.GetLocation();
					// Simple raycast to threat
					FVector Dir = (Threat->GetActorLocation() - MuzzleLoc).GetSafeNormal();
					FHitResult HitResult;
					FCollisionQueryParams Params;
					Params.AddIgnoredActor(Soldier);
					Params.AddIgnoredActor(Threat);
					
					bool bHit = Soldier->GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleLoc, Threat->GetActorLocation(), ECC_Visibility, Params);
					if (bHit && !Cast<ASoldierCharacter>(HitResult.GetActor())) // Hit something static
					{
						D.BlockedShotCount++;
						if (D.BlockedShotCount >= MaxBlockedShots)
						{
							AI->StopCombatPresentation();
							if (bActuallyInCover)
							{
								Soldier->SetCoverState(true, Soldier->IsCurrentCoverLow());
							}
							else
							{
								Soldier->ClearCover();
							}
							return EStateTreeRunStatus::Succeeded; // Exit to trigger reposition
						}
					}
					else 
					{
						D.BlockedShotCount = 0; // Clear on clear shot
					}
				}
			}
		}

		// Do NOT call StartBurst every tick here. StartBurst resets the burst shot count
		// and restarts the firing montage, which creates visible aim/upper-body snapping
		// between shots. The burst is started once when entering Peeking; while peeking we
		// only keep the aim target updated.

		// Time's up or suppressed mid-peek → duck
		if (D.PhaseTimer >= D.CurrentPeekDuration || AI->GetSuppression() > SupThreshold * 1.2f)
		{
			D.Phase = EEngagePhase::Cooldown;
			D.PhaseTimer = 0.f;
			D.bStartedFireForCurrentPeek = false;

			AI->StopCombatPresentation();
			if (bActuallyInCover)
			{
				// Duck back only when real cover exists.
				Soldier->SetCoverState(true, Soldier->IsCurrentCoverLow());
			}
			else
			{
				// Open-ground combat must not fake cover/crouch.
				Soldier->ClearCover();
			}
			D.CyclesCompleted++;

			// Randomize durations for next cycle (each peek feels different)
			D.CurrentPeekDuration = FMath::RandRange(PeekDurationMin, PeekDurationMax);
			D.CurrentCooldownDuration = FMath::RandRange(CooldownMin, CooldownMax);
		}
		break;
	}

	// ---- COOLDOWN (behind cover, waiting) ----
	case EEngagePhase::Cooldown:
	{
		if (D.PhaseTimer >= D.CurrentCooldownDuration)
		{
			if (D.CyclesCompleted >= MaxCycles)
			{
				return EStateTreeRunStatus::Succeeded; // Done — StateTree decides what's next
			}

			D.Phase = EEngagePhase::Waiting;
			D.PhaseTimer = 0.f;
			D.bStartedFireForCurrentPeek = false;
		}
		break;
	}
	}

	return EStateTreeRunStatus::Running;
}

void FSTTask_EngageTarget::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	ASoldierAIController* AI = Cast<ASoldierAIController>(Context.GetOwner());
	if (AI && AI->GetPawn())
	{
		if (ASoldierCharacter* S = Cast<ASoldierCharacter>(AI->GetPawn()))
		{
			AI->StopCombatPresentation();
			if (AI->bHasCover && S->bIsInCover)
			{
				S->SetCoverState(true, S->IsCurrentCoverLow());
			}
			else
			{
				S->ClearCover();
			}
		}
	}
}
