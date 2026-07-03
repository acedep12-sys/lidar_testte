// LeaderCharacter.h — Squad leader: follows quest waypoints and paces speed
#pragma once

#include "CoreMinimal.h"
#include "Characters/SoldierCharacter.h"
#include "SquadTypes.h"
#include "Characters/SquadLeaderTypes.h"
#include "LeaderCharacter.generated.h"

UENUM(BlueprintType)
enum class ELeaderWaypointState : uint8
{
	NotReached,
	Moving,
	Arrived,
	WaitingForClear,
	Completed
};

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API ALeaderCharacter : public ASoldierCharacter
{
	GENERATED_BODY()

public:
	ALeaderCharacter();

	// ---- Pacing ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leader|Pacing")
	float WaitForPlayerRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leader|Pacing")
	float FullSpeedRadius = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leader|Pacing")
	float NormalSpeed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leader|Pacing")
	float WaitingSpeed = 120.f;

	// ---- Waypoints ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leader|Quest")
	TArray<FQuestWaypoint> QuestWaypoints;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Quest")
	int32 CurrentWaypointIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Quest")
	ELeaderWaypointState WaypointState = ELeaderWaypointState::NotReached;

	// ---- API ----
	UFUNCTION(BlueprintCallable, Category = "Leader")
	void SetQuestWaypoints(const TArray<FQuestWaypoint>& Waypoints);

	UFUNCTION(BlueprintCallable, Category = "Leader")
	void BeginQuest();

	UFUNCTION(BlueprintCallable, Category = "Leader")
	bool AdvanceToNextWaypoint();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader")
	bool HasReachedCurrentWaypoint() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader")
	bool IsCurrentAreaClear() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader")
	FVector GetCurrentWaypointLocation() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader")
	float GetPlayerDistance() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader")
	bool IsQuestComplete() const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leader|Presentation")
	ESquadAnimationMode AnimationMode = ESquadAnimationMode::PackDriven;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Presentation")
	ESquadCombatPresentationState PresentationState = ESquadCombatPresentationState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Presentation")
	bool bPresentationCombatReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Presentation")
	bool bPresentationAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Presentation")
	bool bPresentationFiring = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Presentation")
	bool bPresentationHasQuest = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Presentation")
	FVector PresentationMoveGoal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Presentation")
	float PresentationPlayerDistance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Presentation")
	bool bPresentationWaitingForPlayer = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Presentation")
	bool bPresentationReachedWaypoint = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Presentation")
	bool bPresentationAreaClear = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Presentation")
	int32 PresentationWaypointIndex = INDEX_NONE;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	FLeaderQuestProgressSnapshot GetQuestProgressSnapshot() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool IsWaitingForPlayer() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool ShouldUsePackAnimationMode() const { return AnimationMode == ESquadAnimationMode::PackDriven; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool IsPresentationCombatReady() const { return bPresentationCombatReady; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool IsPresentationAiming() const { return bPresentationAiming; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool IsPresentationFiring() const { return bPresentationFiring; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool HasPresentationQuest() const { return bPresentationHasQuest; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	FVector GetPresentationMoveGoal() const { return PresentationMoveGoal; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	ESquadCombatPresentationState GetPresentationState() const { return PresentationState; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool HasValidWaypointPresentation() const { return PresentationWaypointIndex != INDEX_NONE && QuestWaypoints.IsValidIndex(PresentationWaypointIndex); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	ELeaderWaypointState GetWaypointState() const { return WaypointState; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	int32 GetPresentationWaypointIndex() const { return PresentationWaypointIndex; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	float GetPresentationPlayerDistance() const { return PresentationPlayerDistance; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool IsPresentationWaitingForPlayer() const { return bPresentationWaitingForPlayer; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool HasReachedWaypointPresentation() const { return bPresentationReachedWaypoint; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool IsPresentationAreaClear() const { return bPresentationAreaClear; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool IsPresentationQuestComplete() const { return WaypointState == ELeaderWaypointState::Completed; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool IsPresentationMovingToWaypoint() const { return WaypointState == ELeaderWaypointState::Moving || WaypointState == ELeaderWaypointState::NotReached; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool HasPresentationMoveGoal() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool HasPresentationWaypointIndex() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool IsPresentationFollowingMissionPath() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool IsPresentationHoldingForCombat() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	bool IsPresentationAtMissionEnd() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader|Presentation")
	FText GetPresentationDebugStateText() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	float LingerTimer = 0.f;
	float PresentationUpdateAccumulator = 0.f;
	void UpdatePacing();
	void UpdateWaypointState(float DeltaTime);
	void UpdatePresentationState();
	float GetPresentationUpdateInterval() const;
};
