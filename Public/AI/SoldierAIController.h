// SoldierAIController.h — Base AI controller
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/SquadAICommandTypes.h"
#include "SquadTypes.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "GameplayTasksComponent.h"
#include "SoldierAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UStateTreeAIComponent;
class ASoldierCharacter;

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API ASoldierAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASoldierAIController(const FObjectInitializer& OI = FObjectInitializer::Get());

	// ---- Mission Data (Direct Drive) ----
	UPROPERTY(BlueprintReadWrite, Category = "AI|Mission")
	FVector CurrentGoalLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "AI|Mission")
	bool bHasActiveGoal = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Commands")
	FSquadAIMoveCommand ActiveMoveCommand;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Commands")
	FSquadAIAimCommand ActiveAimCommand;

	UFUNCTION(BlueprintCallable, Category = "AI|Commands")
	void SetMoveCommand(const FSquadAIMoveCommand& NewCommand);

	UFUNCTION(BlueprintCallable, Category = "AI|Commands")
	void ClearMoveCommand();

	UFUNCTION(BlueprintCallable, Category = "AI|Commands")
	void SetAimCommand(const FSquadAIAimCommand& NewCommand);

	UFUNCTION(BlueprintCallable, Category = "AI|Commands")
	void ClearAimCommand();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Commands")
	FVector GetEffectiveMoveGoal() const;

	UFUNCTION(BlueprintCallable, Category = "AI|Commands")
	void SetCombatAimTarget(FVector AimLocation, ESquadAIAimMode AimMode = ESquadAIAimMode::CombatTarget, AActor* TargetActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "AI|Commands")
	bool TryStartBurstAtCurrentAim();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Commands")
	bool IsBurstFiring() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Commands")
	bool HasCombatAimCommand() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Commands")
	bool CanUseAdapterPresentation() const;

	UFUNCTION(BlueprintCallable, Category = "AI|Commands")
	void StopCombatPresentation();

	// Persistent movement bookkeeping for StateTree tasks. Do not keep this only in
	// StateTree instance data, because some StateTree setups can re-enter/recreate
	// task instance data and spam MoveTo every tick.
	UPROPERTY(BlueprintReadOnly, Category = "AI|Mission")
	FVector LastMissionMoveGoal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Mission")
	float LastMissionMoveRequestTime = -1000.f;

	// ---- Team Logic ----
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
	virtual void SetGenericTeamId(const FGenericTeamId& InID) override { TeamId = InID; }
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FGenericTeamId TeamId;

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> PerceptionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UGameplayTasksComponent> TasksComp;

	// ---- Suppression ----
	UPROPERTY(BlueprintReadOnly, Category = "AI|Suppression")
	float Suppression = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Suppression")
	float SuppressionDecayPerSec = 0.4f;

	UFUNCTION(BlueprintCallable, Category = "AI|Suppression")
	void AddSuppression(float Amount);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Suppression")
	float GetSuppression() const { return Suppression; }

	// ---- Threat Memory ----
	UPROPERTY(BlueprintReadOnly, Category = "AI|Memory")
	TArray<FPerceivedTarget> Memory;

	// Safety net: keeps combat responsive even if a StateTree asset is missing the
	// threat transitions/evaluators or initial perception events were missed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	bool bEnableDirectCombatFallback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float DirectCombatRange = 9000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float DirectCombatBurstCooldown = 1.25f;

	// Now that StateTree threat transitions work, direct fallback should mainly feed memory/aim.
	// Keep this false for production so AI does not shoot while it is supposed to be moving to cover.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	bool bAllowDirectCombatFallbackFire = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float DirectCombatFallbackFireRange = 4500.f;

	// At very close range, force recognition even if the Visibility trace is blocked by
	// character attachments/capsules/terrain seams. Prevents soldiers standing idle face-to-face.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float DirectCombatCloseRangeOverride = 1600.f;

	UFUNCTION(BlueprintCallable, Category = "AI|Memory")
	AActor* GetPrimaryThreat(float& OutConfidence) const;

	UFUNCTION(BlueprintCallable, Category = "AI|Memory")
	TArray<AActor*> GetCurrentlyVisibleHostiles() const;

	// ---- Cover ----
	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	FCoverPoint PendingCoverResult;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	bool bHasPendingCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	bool bCoverQueryFinished = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	bool bLastCoverQuerySucceeded = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	FCoverPoint CurrentCover;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	bool bHasCover = false;

	// Controller-level cover movement state. StateTree tasks may be re-entered by
	// global transitions while a threat is visible; keeping the goal here prevents
	// TakeCover from restarting the query every frame and never issuing movement.
	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	FVector ActiveCoverMoveGoal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	bool bHasActiveCoverMoveGoal = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	bool bActiveCoverMoveUsesRealCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	float ActiveCoverMoveStartedTime = -1000.f;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	float LastCoverRequestTime = -1000.f;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	float LastTakeCoverCompletedTime = -1000.f;

	// If a StateTree has a broad/global "HasVision -> TakeCover" transition, this cooldown
	// prevents it from re-entering TakeCover every frame and starving Engage.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Cover")
	float TakeCoverReentryCooldown = 8.0f;

	UFUNCTION(BlueprintCallable, Category = "AI|Cover")
	void RequestCover(FVector ThreatLocation, float SearchRadius = 2000.f);

	UFUNCTION(BlueprintCallable, Category = "AI|Cover")
	void SetActiveCoverMoveGoal(FVector GoalLocation, bool bUsesRealCover);

	UFUNCTION(BlueprintCallable, Category = "AI|Cover")
	void ClearActiveCoverMoveGoal(bool bMarkTakeCoverCompleted = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Cover")
	bool HasActiveCoverMoveGoal() const { return bHasActiveCoverMoveGoal; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Cover")
	FVector GetActiveCoverMoveGoal() const { return ActiveCoverMoveGoal; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Cover")
	bool IsActiveCoverMoveUsingRealCover() const { return bActiveCoverMoveUsesRealCover; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Cover")
	float GetActiveCoverMoveStartedTime() const { return ActiveCoverMoveStartedTime; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Cover")
	float GetLastCoverRequestTime() const { return LastCoverRequestTime; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Cover")
	float GetLastTakeCoverCompletedTime() const { return LastTakeCoverCompletedTime; }

	// ---- LOD & Performance ----
	UPROPERTY(BlueprintReadOnly, Category = "AI|LOD")
	int32 CurrentLODBucket = 0;

	UPROPERTY(BlueprintReadOnly, Category = "AI|LOD")
	float SignificanceUpdateMultiplier = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "AI|LOD")
	bool bSkipExpensivePerceptionPolling = false;

	UPROPERTY(BlueprintReadWrite, Category = "AI|LOD")
	bool bForceFullTick = false;

	float ForceFullTickUntil = 0.f;

	// ---- Helpers ----
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI")
	ASoldierCharacter* GetSoldierCharacter() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI")
	bool HasLineOfSightTo(AActor* Target) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Combat")
	bool HasCombatThreat(float MinConfidence = 0.25f) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Combat")
	bool HasNearbyHostile(float Range) const;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void SetupPerception();
	void UpdateMemory(float DeltaTime);
	void PollPerceptionForHostiles();
	void PollRegistryForHostiles();
	void UpsertPerceivedHostile(AActor* Actor, bool bCurrentlyVisible);
	void RunDirectCombatFallback(float DeltaTime);

private:
	UPROPERTY() TObjectPtr<UAISenseConfig_Sight>   SightConfig;
	UPROPERTY() TObjectPtr<UAISenseConfig_Hearing>  HearingConfig;
	UPROPERTY() TObjectPtr<UAISenseConfig_Damage>   DamageConfig;

	float MemoryDecayRate = 0.2f;
	float LastDirectCombatBurstTime = -1000.f;
};
