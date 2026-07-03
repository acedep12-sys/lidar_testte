// AllySoldier.h — Ally: immortal, PowerLevel-driven accuracy, follows leader
// PowerLevel (0-1) is the single master dial that controls everything
#pragma once

#include "CoreMinimal.h"
#include "Characters/SoldierCharacter.h"
#include "Characters/SquadLeaderTypes.h"
#include "AllySoldier.generated.h"

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API AAllySoldier : public ASoldierCharacter
{
	GENERATED_BODY()

public:
	AAllySoldier();

	// ---- The ONE knob that controls ally strength ----

	// 0 = barely hits, 1 = elite. Default 0.25 = shoots often, hits rarely
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Power",
		meta = (ClampMin = "0", ClampMax = "1"))
	float PowerLevel = 0.25f;

	// Per-stat overrides. -1 = derive from PowerLevel. Set >= 0 to override.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Power")
	float AccuracyOverride = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Power")
	float DamageMultiplierOverride = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Power")
	float FireRateMultiplierOverride = -1.f;

	// ---- Formation ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Formation")
	int32 FormationSlot = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Formation")
	TWeakObjectPtr<AActor> LeaderOverride = nullptr;

	// ---- API ----

	// Applies PowerLevel to weapon + health. Called in BeginPlay and by DifficultySubsystem.
	UFUNCTION(BlueprintCallable, Category = "Ally")
	void ApplyPowerLevel();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Presentation")
	ESquadAnimationMode AnimationMode = ESquadAnimationMode::PackDriven;

	UPROPERTY(BlueprintReadOnly, Category = "Ally|Presentation")
	ESquadCombatPresentationState PresentationState = ESquadCombatPresentationState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Ally|Presentation")
	bool bPresentationCombatReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Ally|Presentation")
	bool bPresentationAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Ally|Presentation")
	bool bPresentationFiring = false;

	UPROPERTY(BlueprintReadOnly, Category = "Ally|Presentation")
	bool bPresentationHasLeader = false;

	UPROPERTY(BlueprintReadOnly, Category = "Ally|Presentation")
	FVector PresentationLeaderLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Ally|Presentation")
	float PresentationLeaderDistance = 0.f;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	bool HasLeaderPresentation() const { return bPresentationHasLeader; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	FVector GetPresentationLeaderLocation() const { return PresentationLeaderLocation; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	float GetPresentationLeaderDistance() const { return PresentationLeaderDistance; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	bool IsPresentationCombatReady() const { return bPresentationCombatReady; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	bool IsPresentationAiming() const { return bPresentationAiming; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	bool IsPresentationFiring() const { return bPresentationFiring; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	ESquadCombatPresentationState GetPresentationState() const { return PresentationState; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	int32 GetFormationSlot() const { return FormationSlot; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	bool HasLeaderOverrideAssigned() const { return LeaderOverride.IsValid(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	AActor* GetLeaderOverrideActor() const { return LeaderOverride.Get(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	bool HasLeaderDistancePresentation() const { return bPresentationHasLeader && PresentationLeaderDistance > 0.f; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	bool ShouldUsePackAnimationMode() const { return AnimationMode == ESquadAnimationMode::PackDriven; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	bool IsPresentationFollowingLeader() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	bool IsPresentationHoldingForCombat() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ally|Presentation")
	FText GetPresentationDebugStateText() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	float PresentationUpdateAccumulator = 0.f;
	void UpdatePresentationState();
	float GetPresentationUpdateInterval() const;
};
