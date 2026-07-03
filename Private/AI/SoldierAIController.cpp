#include "AI/SoldierAIController.h"
#include "Actors/SquadAIAutoSetupManager.h"
#include "Characters/SoldierCharacter.h"
#include "CoverSystem/CoverSystemSubsystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Components/StateTreeAIComponent.h"
#include "Components/AimComponent.h"
#include "Components/WeaponComponent.h"
#include "Components/SquadAIAdapterComponent.h"
#include "BlueprintLibraries/SquadAIPackAssetResolverLibrary.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "SquadAITuning.h"
#include "Performance/SharedThreatMemory.h"
#include "Performance/AISignificanceManager.h"
#include "Performance/SoldierRegistrySubsystem.h"
#include "Quest/QuestRegistry.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayTasksComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

ASoldierAIController::ASoldierAIController(const FObjectInitializer& OI)
	: Super(OI.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	bWantsPlayerState = true; // Crucial for Lyra Ability System and Team integration
	
	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*PerceptionComp);
	StateTreeComp = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	TasksComp = CreateDefaultSubobject<UGameplayTasksComponent>(TEXT("GameplayTasks"));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f; 
	TeamId = FGenericTeamId(255);
}

void ASoldierAIController::BeginPlay()
{
	Super::BeginPlay();
	const USquadAITuning* T = USquadAITuning::Get();
	MemoryDecayRate = T->MemoryDecayPerSecond;
	SuppressionDecayPerSec = T->SuppressionDecayPerSecond;
	DirectCombatRange = T->CombatDetectionRange;
	DirectCombatCloseRangeOverride = T->CombatCloseRangeOverride;
	DirectCombatFallbackFireRange = T->DirectFallbackFireRange;
	bAllowDirectCombatFallbackFire = T->bAllowDirectFallbackFire;

	// Configure perception after the listener has been registered. Calling ConfigureSense
	// too early produces "Listener must have a valid id" and can make perception events unreliable.
	FTimerHandle PerceptionSetupHandle;
	GetWorldTimerManager().SetTimer(PerceptionSetupHandle, this, &ASoldierAIController::SetupPerception, 1.0f, false);
}

void ASoldierAIController::OnPossess(APawn* InPawn)
{
	// Do NOT copy the pawn's cached TeamId here. For placed AI, possession can happen
	// before BeginPlay, so ASoldierCharacter::BeginPlay may not have populated TeamId yet.
	// Faction is already initialized from C++/Blueprint defaults, so derive the team from it.
	if (ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(InPawn))
	{
		SetGenericTeamId(Soldier->Faction == ESquadFaction::Enemy ? FGenericTeamId(1) : FGenericTeamId(0));
		Soldier->SetGenericTeamId(GetGenericTeamId());

		// Check if BPC_AIDetection_Master is missing on the controller, as TPWCS AI controllers require it.
		bool bHasDetection = false;
		TArray<UActorComponent*> ControllerComponents;
		GetComponents(ControllerComponents);
		for (UActorComponent* Comp : ControllerComponents)
		{
			if (Comp && (Comp->GetName().Contains(TEXT("BPC_AIDetection")) || (Comp->GetClass() && Comp->GetClass()->GetName().Contains(TEXT("BPC_AIDetection")))))
			{
				bHasDetection = true;
				break;
			}
		}

		if (!bHasDetection)
		{
			UClass* DetectionClass = nullptr;
#if WITH_EDITOR
			DetectionClass = USquadAIPackAssetResolverLibrary::ResolvePackComponentClassByName(TEXT("BPC_AIDetection_Master"));
#endif
			if (!DetectionClass)
			{
				DetectionClass = StaticLoadClass(UActorComponent::StaticClass(), nullptr, TEXT("/Game/ThirdPersonMP/Blueprints/Components/BPC_AIDetection_Master.BPC_AIDetection_Master_C"));
			}

			if (DetectionClass)
			{
				UActorComponent* NewDetectionComp = NewObject<UActorComponent>(this, DetectionClass, TEXT("BPC_AIDetection_Master_Runtime"));
				if (NewDetectionComp)
				{
					NewDetectionComp->RegisterComponent();
					AddInstanceComponent(NewDetectionComp);
					UE_LOG(LogTemp, Log, TEXT("[SoldierAIController] Successfully added runtime BPC_AIDetection_Master to controller %s"), *GetNameSafe(this));
				}
			}
		}
	}

	Super::OnPossess(InPawn);
	if (StateTreeComp) StateTreeComp->RestartLogic();
	if (PerceptionComp) PerceptionComp->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ASoldierAIController::OnPerceptionUpdated);
}

ETeamAttitude::Type ASoldierAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);

	// If the perceived actor is a pawn, its controller may be the actual team agent.
	if (!OtherTeamAgent)
	{
		if (const APawn* OtherPawn = Cast<const APawn>(&Other))
		{
			OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(OtherPawn->GetController());
		}
	}

	if (!OtherTeamAgent)
	{
		// Pack player characters usually do not implement SquadAI's team interface.
		// Treat the local/player-controlled pawn as team 0 (player/allied side) so enemies
		// can actually perceive and fight the player, while allies/leaders consider the
		// player friendly. Without this, pack player pawns become Neutral and enemies will
		// crouch/idle instead of shooting at the player.
		if (const APawn* OtherPawn = Cast<const APawn>(&Other))
		{
			if (OtherPawn->IsPlayerControlled())
			{
				return TeamId == FGenericTeamId(1) ? ETeamAttitude::Hostile : ETeamAttitude::Friendly;
			}
		}

		return ETeamAttitude::Neutral;
	}

	const FGenericTeamId OtherTeamId = OtherTeamAgent->GetGenericTeamId();
	if (TeamId == FGenericTeamId::NoTeam || OtherTeamId == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	return TeamId == OtherTeamId ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
}

void ASoldierAIController::OnUnPossess()
{
	if (PerceptionComp) PerceptionComp->OnTargetPerceptionUpdated.RemoveDynamic(this, &ASoldierAIController::OnPerceptionUpdated);
	Super::OnUnPossess();
}

void ASoldierAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ASoldierCharacter* Soldier = GetSoldierCharacter();
	const float EffectiveDeltaTime = DeltaTime * FMath::Max(SignificanceUpdateMultiplier, 0.1f);
	const float CoverDecayMult = (Soldier && Soldier->bIsInCover) ? 0.5f : 1.f;
	Suppression = FMath::Max(0.f, Suppression - SuppressionDecayPerSec * EffectiveDeltaTime * CoverDecayMult);
	UpdateMemory(EffectiveDeltaTime);
	if (!bSkipExpensivePerceptionPolling)
	{
		PollPerceptionForHostiles();
		PollRegistryForHostiles();
	}
	RunDirectCombatFallback(EffectiveDeltaTime);
}

void ASoldierAIController::SetMoveCommand(const FSquadAIMoveCommand& NewCommand)
{
	ActiveMoveCommand = NewCommand;
	CurrentGoalLocation = NewCommand.GoalLocation;
	bHasActiveGoal = NewCommand.IsValid();
	if (ASoldierCharacter* Soldier = GetSoldierCharacter())
	{
		if (Soldier->AIAdapterComp)
		{
			Soldier->AIAdapterComp->SetMoveCommand(NewCommand);
		}
	}
}

void ASoldierAIController::ClearMoveCommand()
{
	ActiveMoveCommand = FSquadAIMoveCommand();
	CurrentGoalLocation = FVector::ZeroVector;
	bHasActiveGoal = false;
	if (ASoldierCharacter* Soldier = GetSoldierCharacter())
	{
		if (Soldier->AIAdapterComp)
		{
			Soldier->AIAdapterComp->ClearMoveCommand();
		}
	}
}

void ASoldierAIController::SetAimCommand(const FSquadAIAimCommand& NewCommand)
{
	ActiveAimCommand = NewCommand;
	if (ASoldierCharacter* Soldier = GetSoldierCharacter())
	{
		if (Soldier->AIAdapterComp)
		{
			Soldier->AIAdapterComp->SetAimCommand(NewCommand);
		}
	}
}

void ASoldierAIController::ClearAimCommand()
{
	ActiveAimCommand = FSquadAIAimCommand();
	if (ASoldierCharacter* Soldier = GetSoldierCharacter())
	{
		if (Soldier->AIAdapterComp)
		{
			Soldier->AIAdapterComp->ClearAimCommand();
		}
	}
}

FVector ASoldierAIController::GetEffectiveMoveGoal() const
{
	if (ActiveMoveCommand.IsValid())
	{
		return ActiveMoveCommand.GoalLocation;
	}
	if (!CurrentGoalLocation.IsNearlyZero())
	{
		return CurrentGoalLocation;
	}
	if (const UQuestRegistry* GPS = GetWorld() ? GetWorld()->GetSubsystem<UQuestRegistry>() : nullptr)
	{
		return GPS->CurrentActiveGoal;
	}
	return FVector::ZeroVector;
}

void ASoldierAIController::SetCombatAimTarget(FVector AimLocation, ESquadAIAimMode AimMode, AActor* TargetActor)
{
	FSquadAIAimCommand AimCommand;
	AimCommand.Mode = AimMode;
	AimCommand.AimLocation = AimLocation;
	AimCommand.TargetActor = TargetActor;
	SetAimCommand(AimCommand);

	// Pack-driven characters do not use the old SquadAI aim offset/montage stack, so make
	// the pawn itself face its combat target. This fixes firing while visually looking away.
	if (ASoldierCharacter* Soldier = GetSoldierCharacter())
	{
		if (Soldier->IsUsingPackPresentationMode())
		{
			const FVector ToAim = AimLocation - Soldier->GetActorLocation();
			const FVector ToAim2D(ToAim.X, ToAim.Y, 0.f);
			if (!ToAim2D.IsNearlyZero())
			{
				const FRotator DesiredYaw = ToAim2D.Rotation();
				Soldier->SetActorRotation(FRotator(0.f, DesiredYaw.Yaw, 0.f));
				SetControlRotation(FRotator(0.f, DesiredYaw.Yaw, 0.f));
			}
		}
	}
}

bool ASoldierAIController::TryStartBurstAtCurrentAim()
{
	if (ASoldierCharacter* Soldier = GetSoldierCharacter())
	{
		if (Soldier->AIAdapterComp)
		{
			return Soldier->AIAdapterComp->TryStartBurstAtAimCommand();
		}
	}
	return false;
}

bool ASoldierAIController::IsBurstFiring() const
{
	if (const ASoldierCharacter* Soldier = GetSoldierCharacter())
	{
		if (Soldier->AIAdapterComp)
		{
			return Soldier->AIAdapterComp->IsBurstFiring();
		}
	}
	return false;
}

bool ASoldierAIController::HasCombatAimCommand() const
{
	if (const ASoldierCharacter* Soldier = GetSoldierCharacter())
	{
		if (Soldier->AIAdapterComp)
		{
			return Soldier->AIAdapterComp->HasAimCommand();
		}
	}
	return false;
}

bool ASoldierAIController::CanUseAdapterPresentation() const
{
	if (const ASoldierCharacter* Soldier = GetSoldierCharacter())
	{
		return Soldier->AIAdapterComp != nullptr;
	}
	return false;
}

void ASoldierAIController::StopCombatPresentation()
{
	ClearAimCommand();
	if (ASoldierCharacter* Soldier = GetSoldierCharacter())
	{
		if (Soldier->AIAdapterComp)
		{
			Soldier->AIAdapterComp->StopBurstAndClearAim();
		}
	}
}

void ASoldierAIController::SetupPerception()
{
	if (!PerceptionComp) return;
	const USquadAITuning* T = USquadAITuning::Get();
	SightConfig = NewObject<UAISenseConfig_Sight>(this);
	SightConfig->SightRadius = T->SightRadius;
	SightConfig->LoseSightRadius = T->LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = T->PeripheralVisionDegrees;
	SightConfig->DetectionByAffiliation.bDetectEnemies = SightConfig->DetectionByAffiliation.bDetectNeutrals = SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	PerceptionComp->ConfigureSense(*SightConfig);
	HearingConfig = NewObject<UAISenseConfig_Hearing>(this);
	HearingConfig->HearingRange = T->HearingRange;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = HearingConfig->DetectionByAffiliation.bDetectNeutrals = HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	PerceptionComp->ConfigureSense(*HearingConfig);
	DamageConfig = NewObject<UAISenseConfig_Damage>(this);
	PerceptionComp->ConfigureSense(*DamageConfig);
}

void ASoldierAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !Cast<APawn>(Actor)) return;
	bool bIsAlive = true;
	if (ASoldierCharacter* S = Cast<ASoldierCharacter>(Actor)) bIsAlive = S->IsAlive();
	if (!bIsAlive) return;

	// Use this controller's team attitude check.
	const ETeamAttitude::Type Attitude = GetTeamAttitudeTowards(*Actor);
	const TCHAR* AttitudeText = TEXT("Neutral");
	if (Attitude == ETeamAttitude::Hostile)
	{
		AttitudeText = TEXT("Hostile");
	}
	else if (Attitude == ETeamAttitude::Friendly)
	{
		AttitudeText = TEXT("Friendly");
	}

	if (ASquadAIAutoSetupManager::AreVerboseDebugLogsEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SquadAIPerceptionDebug] Pawn=%s ControllerTeam=%d Perceived=%s Class=%s Sensed=%d Attitude=%s StimulusAge=%.2f Strength=%.2f"),
			*GetNameSafe(GetPawn()),
			GetGenericTeamId().GetId(),
			*GetNameSafe(Actor),
			*GetNameSafe(Actor->GetClass()),
			Stimulus.WasSuccessfullySensed() ? 1 : 0,
			AttitudeText,
			Stimulus.GetAge(),
			Stimulus.Strength);
	}

	if (Attitude != ETeamAttitude::Hostile) return;

	UpsertPerceivedHostile(Actor, Stimulus.WasSuccessfullySensed());
}

void ASoldierAIController::UpdateMemory(float DeltaTime)
{
	for (int32 i = Memory.Num() - 1; i >= 0; --i) {
		FPerceivedTarget& T = Memory[i];
		if (!T.Actor.IsValid() || (Cast<ASoldierCharacter>(T.Actor.Get()) && !Cast<ASoldierCharacter>(T.Actor.Get())->IsAlive())) {
			Memory.RemoveAtSwap(i); continue;
		}
		if (!T.bCurrentlyVisible) {
			T.Confidence -= MemoryDecayRate * DeltaTime;
			if (T.Confidence <= 0.f) Memory.RemoveAtSwap(i);
		} else T.LastKnownLocation = T.Actor->GetActorLocation();
	}
}

void ASoldierAIController::UpsertPerceivedHostile(AActor* Actor, bool bCurrentlyVisible)
{
	if (!Actor || !GetPawn()) return;

	FPerceivedTarget* Existing = Memory.FindByPredicate([Actor](const FPerceivedTarget& T) { return T.Actor.Get() == Actor; });
	if (Existing)
	{
		Existing->LastKnownLocation = Actor->GetActorLocation();
		Existing->Confidence = bCurrentlyVisible ? 1.f : Existing->Confidence;
		Existing->bCurrentlyVisible = bCurrentlyVisible;
		if (ASquadAIAutoSetupManager::AreVerboseDebugLogsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("[SquadAIMemoryDebug] Update Pawn=%s Hostile=%s Visible=%d Conf=%.2f MemoryNum=%d"), *GetNameSafe(GetPawn()), *GetNameSafe(Actor), bCurrentlyVisible ? 1 : 0, Existing->Confidence, Memory.Num());
		}
	}
	else if (bCurrentlyVisible)
	{
		FPerceivedTarget NewTarget;
		NewTarget.Actor = Actor;
		NewTarget.LastKnownLocation = Actor->GetActorLocation();
		NewTarget.Confidence = 1.f;
		NewTarget.bCurrentlyVisible = true;
		Memory.Add(NewTarget);
		if (ASquadAIAutoSetupManager::AreVerboseDebugLogsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("[SquadAIMemoryDebug] Add Pawn=%s Hostile=%s Visible=1 Conf=1.00 MemoryNum=%d"), *GetNameSafe(GetPawn()), *GetNameSafe(Actor), Memory.Num());
		}
	}
	else
	{
		if (ASquadAIAutoSetupManager::AreVerboseDebugLogsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("[SquadAIMemoryDebug] IgnoreNonVisibleNew Pawn=%s Actor=%s"), *GetNameSafe(GetPawn()), *GetNameSafe(Actor));
		}
	}
}

void ASoldierAIController::PollPerceptionForHostiles()
{
	if (!PerceptionComp || !GetPawn()) return;

	TArray<AActor*> VisibleActors;
	PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), VisibleActors);

	for (AActor* Actor : VisibleActors)
	{
		if (!Actor || Actor == GetPawn() || !Cast<APawn>(Actor)) continue;

		if (ASoldierCharacter* S = Cast<ASoldierCharacter>(Actor))
		{
			if (!S->IsAlive()) continue;
		}

		if (GetTeamAttitudeTowards(*Actor) == ETeamAttitude::Hostile)
		{
			UpsertPerceivedHostile(Actor, true);
		}
	}
}

void ASoldierAIController::PollRegistryForHostiles()
{
	if (!GetPawn()) return;

	USoldierRegistrySubsystem* Registry = GetWorld()->GetSubsystem<USoldierRegistrySubsystem>();
	if (!Registry) return;

	TArray<ASoldierCharacter*> Nearby = Registry->QueryRadius(GetPawn()->GetActorLocation(), DirectCombatRange);
	for (ASoldierCharacter* Other : Nearby)
	{
		if (!Other || Other == GetPawn() || !Other->IsAlive()) continue;

		const bool bDifferentTeam = Other->GetGenericTeamId().GetId() != GetGenericTeamId().GetId();
		if (!bDifferentTeam) continue;

		// Registry is the cheap spatial broad phase. Line of sight is the narrow phase.
		// The close-range override prevents soldiers standing idle face-to-face if the
		// Visibility trace is blocked by attachments/capsules/terrain seams.
		const float Dist = FVector::Dist(GetPawn()->GetActorLocation(), Other->GetActorLocation());
		if (HasLineOfSightTo(Other) || Dist <= DirectCombatCloseRangeOverride)
		{
			UpsertPerceivedHostile(Other, true);
		}
	}
}

void ASoldierAIController::RunDirectCombatFallback(float DeltaTime)
{
	if (!bEnableDirectCombatFallback || !GetPawn()) return;

	ASoldierCharacter* Soldier = GetSoldierCharacter();
	if (!Soldier) return;

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const bool bPackMode = Soldier->IsUsingPackPresentationMode();
	auto DebugLog = [this](const TCHAR* Reason, AActor* ThreatActor, float ConfidenceValue, float DistanceValue, bool bLOSValue, bool bTacticalValue)
	{
		if (!ASquadAIAutoSetupManager::AreVerboseDebugLogsEnabled())
		{
			return;
		}

		UE_LOG(LogTemp, Warning,
			TEXT("[SquadAICombatDebug] Pawn=%s Controller=%s Team=%d PackMode=%d Reason=%s Threat=%s ThreatClass=%s Conf=%.2f Dist=%.0f LOS=%d HasCover=%d PawnInCover=%d PawnCrouched=%d ActiveCoverMove=%d ActiveCoverReal=%d AllowFallbackFire=%d TacticalAllowed=%d DirectRange=%.0f FireRange=%.0f MemoryNum=%d"),
			*GetNameSafe(GetPawn()),
			*GetNameSafe(this),
			GetGenericTeamId().GetId(),
			GetSoldierCharacter() && GetSoldierCharacter()->IsUsingPackPresentationMode() ? 1 : 0,
			Reason,
			*GetNameSafe(ThreatActor),
			ThreatActor ? *GetNameSafe(ThreatActor->GetClass()) : TEXT("None"),
			ConfidenceValue,
			DistanceValue,
			bLOSValue ? 1 : 0,
			bHasCover ? 1 : 0,
			GetSoldierCharacter() && GetSoldierCharacter()->bIsInCover ? 1 : 0,
			GetSoldierCharacter() && GetSoldierCharacter()->bIsCrouched ? 1 : 0,
			HasActiveCoverMoveGoal() ? 1 : 0,
			IsActiveCoverMoveUsingRealCover() ? 1 : 0,
			bAllowDirectCombatFallbackFire ? 1 : 0,
			bTacticalValue ? 1 : 0,
			DirectCombatRange,
			DirectCombatFallbackFireRange,
			Memory.Num());
	};

	float Confidence = 0.f;
	AActor* Threat = GetPrimaryThreat(Confidence);
	if (!Threat || Confidence < 0.25f)
	{
		DebugLog(TEXT("NoThreatOrLowConfidence"), Threat, Confidence, -1.f, false, false);
		StopCombatPresentation();
		return;
	}

	if (const ASoldierCharacter* ThreatSoldier = Cast<ASoldierCharacter>(Threat))
	{
		if (!ThreatSoldier->IsAlive())
		{
			DebugLog(TEXT("ThreatDead"), Threat, Confidence, -1.f, false, false);
			StopCombatPresentation();
			return;
		}
	}

	const float Dist = FVector::Dist(GetPawn()->GetActorLocation(), Threat->GetActorLocation());
	if (Dist > DirectCombatRange)
	{
		DebugLog(TEXT("ThreatOutsideDirectCombatRange"), Threat, Confidence, Dist, false, false);
		return;
	}

	const bool bHasCombatLOS = HasLineOfSightTo(Threat) || Dist <= DirectCombatCloseRangeOverride;
	if (!bHasCombatLOS)
	{
		DebugLog(TEXT("NoLOS"), Threat, Confidence, Dist, false, false);
		StopCombatPresentation();
		return;
	}

	const FVector AimLoc = Threat->GetActorLocation() + FVector(0.f, 0.f, 70.f);
	SetCombatAimTarget(AimLoc, ESquadAIAimMode::CombatTarget, Threat);

	// Do not freeze pack-driven characters just because they see a threat.
	// TPWCS firing is still being integrated; stopping movement before pack firing is proven
	// made actors stand still doing nothing. Non-pack legacy SquadAI can keep the old safety stop.
	if (!bPackMode && !HasActiveCoverMoveGoal())
	{
		if (UPathFollowingComponent* PFC = GetPathFollowingComponent())
		{
			if (PFC->GetStatus() == EPathFollowingStatus::Moving)
			{
				StopMovement();
			}
		}
	}

	// Fallback fire is only allowed when the AI is actively moving to cover, already in
	// cover, or close enough for a direct fight. This prevents long-range open-field
	// standing fire while still allowing "shoot while moving to cover" behavior.
	const bool bTacticallyAllowedToFallbackFire = HasActiveCoverMoveGoal() || bHasCover || Dist <= 2800.f;
	if ((!bAllowDirectCombatFallbackFire && !bPackMode) || Dist > DirectCombatFallbackFireRange || !bTacticallyAllowedToFallbackFire)
	{
		DebugLog(TEXT("FallbackFireGated"), Threat, Confidence, Dist, bHasCombatLOS, bTacticallyAllowedToFallbackFire);
		return;
	}

	// Do not shoot into sandbags/walls from the actual weapon/muzzle position. Actor-center LOS
	// can be clear while the weapon muzzle is blocked by cover. In that case let cover/reposition
	// logic handle it instead of wasting shots into collision.
	if (Soldier && Soldier->WeaponComp && Threat)
	{
		const FVector MuzzleLoc = Soldier->WeaponComp->GetMuzzleLocation();
		FHitResult WeaponLOSHit;
		FCollisionQueryParams WeaponLOSParams(SCENE_QUERY_STAT(SquadAIWeaponLOS), false);
		WeaponLOSParams.AddIgnoredActor(GetPawn());
		TArray<AActor*> AttachedActors;
		GetPawn()->GetAttachedActors(AttachedActors, true);
		for (AActor* Attached : AttachedActors)
		{
			WeaponLOSParams.AddIgnoredActor(Attached);
		}

		const bool bWeaponBlocked = GetWorld()->LineTraceSingleByChannel(WeaponLOSHit, MuzzleLoc, AimLoc, ECC_Visibility, WeaponLOSParams);
		if (bWeaponBlocked)
		{
			AActor* HitActor = WeaponLOSHit.GetActor();
			bool bHitTargetOrChild = HitActor == Threat;
			for (AActor* Cursor = HitActor; Cursor && !bHitTargetOrChild; Cursor = Cursor->GetAttachParentActor())
			{
				bHitTargetOrChild = Cursor == Threat;
			}
			if (!bHitTargetOrChild)
			{
				// If the eyes can see but the weapon is blocked because the pawn is crouched/low,
				// stand/peek first instead of shooting into the hill/sandbag.
				if (Soldier && Soldier->bIsCrouched)
				{
					if (Soldier->bIsInCover)
					{
						Soldier->SetCoverState(true, false);
					}
					else
					{
						Soldier->ClearCover();
						Soldier->UnCrouch();
					}
				}
				DebugLog(TEXT("WeaponMuzzleBlocked_StandOrReposition"), Threat, Confidence, Dist, false, bTacticallyAllowedToFallbackFire);
				return;
			}
		}
	}

	if (Now - LastDirectCombatBurstTime >= DirectCombatBurstCooldown)
	{
		const bool bStarted = TryStartBurstAtCurrentAim();
		DebugLog(bStarted ? TEXT("TryStartBurstSucceeded") : TEXT("TryStartBurstFailed"), Threat, Confidence, Dist, bHasCombatLOS, bTacticallyAllowedToFallbackFire);
		if (bStarted)
		{
			LastDirectCombatBurstTime = Now;
		}
	}
	else
	{
		DebugLog(TEXT("BurstCooldown"), Threat, Confidence, Dist, bHasCombatLOS, bTacticallyAllowedToFallbackFire);
	}
}

AActor* ASoldierAIController::GetPrimaryThreat(float& OutConfidence) const
{
	OutConfidence = 0.f;
	if (!GetPawn()) return nullptr;

	AActor* Best = nullptr;
	float BestScore = -FLT_MAX;
	for (const FPerceivedTarget& T : Memory)
	{
		AActor* Candidate = T.Actor.Get();
		if (!Candidate) continue;
		if (const ASoldierCharacter* S = Cast<ASoldierCharacter>(Candidate))
		{
			if (!S->IsAlive()) continue;
		}

		float Score = T.Confidence * 1000.f - FVector::DistSquared(T.LastKnownLocation, GetPawn()->GetActorLocation()) * 0.001f;
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Candidate;
			OutConfidence = T.Confidence;
		}
	}
	return Best;
}

TArray<AActor*> ASoldierAIController::GetCurrentlyVisibleHostiles() const
{
	TArray<AActor*> Result;
	for (const FPerceivedTarget& T : Memory) if (T.bCurrentlyVisible && T.Actor.IsValid()) Result.Add(T.Actor.Get());
	return Result;
}

void ASoldierAIController::AddSuppression(float Amount) { Suppression = FMath::Clamp(Suppression + Amount, 0.f, 1.f); }

void ASoldierAIController::SetActiveCoverMoveGoal(FVector GoalLocation, bool bUsesRealCover)
{
	ActiveCoverMoveGoal = GoalLocation;
	bHasActiveCoverMoveGoal = !GoalLocation.IsNearlyZero();
	bActiveCoverMoveUsesRealCover = bUsesRealCover;
	ActiveCoverMoveStartedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

void ASoldierAIController::ClearActiveCoverMoveGoal(bool bMarkTakeCoverCompleted)
{
	ActiveCoverMoveGoal = FVector::ZeroVector;
	bHasActiveCoverMoveGoal = false;
	bActiveCoverMoveUsesRealCover = false;
	ActiveCoverMoveStartedTime = -1000.f;
	if (bMarkTakeCoverCompleted)
	{
		LastTakeCoverCompletedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastTakeCoverCompletedTime;
	}
}

void ASoldierAIController::RequestCover(FVector TL, float R) {
	bHasPendingCover = false;
	bCoverQueryFinished = false;
	bLastCoverQuerySucceeded = false;
	LastCoverRequestTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastCoverRequestTime;
	UCoverSystemSubsystem* CS = GetWorld()->GetSubsystem<UCoverSystemSubsystem>();
	if (!CS || !GetPawn()) {
		bCoverQueryFinished = true;
		bLastCoverQuerySucceeded = false;
		return;
	}
	TWeakObjectPtr<ASoldierAIController> WeakSelf(this);
	CS->RequestCoverAsync(GetPawn(), GetPawn()->GetActorLocation(), TL, R, [WeakSelf](bool bFound, const FCoverPoint& Res) {
		if (WeakSelf.IsValid()) {
			WeakSelf->PendingCoverResult = Res;
			WeakSelf->bHasPendingCover = bFound;
			WeakSelf->bCoverQueryFinished = true;
			WeakSelf->bLastCoverQuerySucceeded = bFound;
		}
	});
}

ASoldierCharacter* ASoldierAIController::GetSoldierCharacter() const { return Cast<ASoldierCharacter>(GetPawn()); }

bool ASoldierAIController::HasNearbyHostile(float Range) const
{
	if (!GetPawn()) return false;

	if (const USoldierRegistrySubsystem* Registry = GetWorld()->GetSubsystem<USoldierRegistrySubsystem>())
	{
		TArray<ASoldierCharacter*> Nearby = Registry->QueryRadius(GetPawn()->GetActorLocation(), Range);
		for (ASoldierCharacter* Other : Nearby)
		{
			if (!Other || Other == GetPawn() || !Other->IsAlive()) continue;
			if (Other->GetGenericTeamId().GetId() == GetGenericTeamId().GetId()) continue;
			return true;
		}
	}

	return false;
}

bool ASoldierAIController::HasCombatThreat(float MinConfidence) const
{
	if (!GetPawn()) return false;

	float Confidence = 0.f;
	AActor* Threat = GetPrimaryThreat(Confidence);
	if (Threat && Confidence >= MinConfidence)
	{
		return true;
	}

	// Fallback broad-phase using the soldier registry. This makes mission/follow tasks
	// stop even if the StateTree evaluator transition has not fired yet this frame.
	if (const USoldierRegistrySubsystem* Registry = GetWorld()->GetSubsystem<USoldierRegistrySubsystem>())
	{
		TArray<ASoldierCharacter*> Nearby = Registry->QueryRadius(GetPawn()->GetActorLocation(), DirectCombatRange);
		for (ASoldierCharacter* Other : Nearby)
		{
			if (!Other || Other == GetPawn() || !Other->IsAlive()) continue;
			if (Other->GetGenericTeamId().GetId() == GetGenericTeamId().GetId()) continue;

			const float Dist = FVector::Dist(GetPawn()->GetActorLocation(), Other->GetActorLocation());
			if (Dist <= DirectCombatCloseRangeOverride || HasLineOfSightTo(Other))
			{
				return true;
			}
		}
	}

	return false;
}

bool ASoldierAIController::HasLineOfSightTo(AActor* Target) const {
	if (!Target || !GetPawn()) return false;
	FHitResult Hit; FCollisionQueryParams P; P.AddIgnoredActor(GetPawn());
	const FVector Start = GetPawn()->GetActorLocation() + FVector(0,0,60);
	const FVector End = Target->GetActorLocation() + FVector(0,0,60);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, P)) return true;

	AActor* HitActor = Hit.GetActor();
	if (HitActor == Target) return true;

	// Pack characters often have attached weapon/mesh/helper actors or components. If LOS
	// hits something attached to the target pawn, treat that as seeing the target instead
	// of falsely failing LOS and forcing crouch/reposition loops.
	for (AActor* Cursor = HitActor; Cursor; Cursor = Cursor->GetAttachParentActor())
	{
		if (Cursor == Target)
		{
			return true;
		}
	}

	return false;
}
