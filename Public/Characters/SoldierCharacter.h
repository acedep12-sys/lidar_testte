// SoldierCharacter.h — Base character for all AI soldiers
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "SquadTypes.h"
#include "SoldierCharacter.generated.h"

class UAimComponent;
class UWeaponComponent;
class UHealthComponent;
class UAIPerceptionStimuliSourceComponent;
class UAnimMontage;
class UMeshComponent;
class USquadAIAdapterComponent;

UENUM(BlueprintType)
enum class ESoldierCoverSide : uint8
{
	Right UMETA(DisplayName = "Right"),
	Left  UMETA(DisplayName = "Left")
};

UENUM(BlueprintType)
enum class ESoldierCoverPeekType : uint8
{
	Side UMETA(DisplayName = "Side"),
	Over UMETA(DisplayName = "Over")
};

USTRUCT(BlueprintType)
struct FWeaponIKRuntimeData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK")
	bool bEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK")
	float Alpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK")
	FVector LeftHandEffectorLocationCS = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK")
	FRotator LeftHandEffectorRotationCS = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK")
	FVector LeftElbowJointTargetBS = FVector(0.f, -80.f, 0.f);

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK")
	FVector LeftElbowJointTargetCS = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK")
	bool bUseDynamicElbowTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK")
	bool bUseLeftHandRotation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK")
	float LeftHandRotationAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK|Rotation Distribution")
	bool bUseLeftForearmAdditiveRotation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK|Rotation Distribution")
	FRotator LeftForearmAdditiveRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK|Rotation Distribution")
	float LeftForearmAdditiveRotationAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK|Rotation Distribution")
	bool bUseLeftHandAdditiveRotation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK|Rotation Distribution")
	FRotator LeftHandAdditiveRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK|Rotation Distribution")
	float LeftHandAdditiveRotationAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK|Grip")
	bool bUseWeaponGripPose = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK|Grip")
	float LeftHandGripAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon IK|Grip")
	float RightHandGripAlpha = 0.f;
};

USTRUCT(BlueprintType)
struct FSoldierAnimRuntimeData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim")
	float Speed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim")
	float Direction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim")
	bool bIsAlive = true;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim")
	bool bIsCrouched = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim")
	bool bCombatReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim")
	bool bWantsToFire = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bIsInCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bIsCrouchingInCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bIsPeekingFromCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bCoverIsLow = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bCoverSideIsLeft = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bCoverPeekIsOver = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bIsMovingInCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bIsAdjustingCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	float CoverMoveDirection = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Aim")
	float AimYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Aim")
	float AimPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	FWeaponIKRuntimeData WeaponIK;
};

UCLASS(Abstract, BlueprintType, Blueprintable)
class SQUADAI_API ASoldierCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ASoldierCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soldier|Components")
	TObjectPtr<UAimComponent> AimComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soldier|Components")
	TObjectPtr<UWeaponComponent> WeaponComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soldier|Components")
	TObjectPtr<UHealthComponent> HealthComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soldier|Components")
	TObjectPtr<USquadAIAdapterComponent> AIAdapterComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soldier|Components")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> PerceptionStimuli;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon Visual")
	TObjectPtr<UMeshComponent> EquippedWeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon Visual")
	FName WeaponAttachSocketName = FName("RifleSocket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon Visual")
	FName WeaponLeftHandIKSocketName = FName("LeftHandIK");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon Visual")
	FName WeaponMuzzleSocketName = FName("Muzzle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon Visual|Attach")
	bool bAutoAttachEquippedWeaponToMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Pack")
	bool bUsePackPresentationMode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon Visual|Attach")
	FVector WeaponAttachLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon Visual|Attach")
	FRotator WeaponAttachRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon Visual|Attach")
	bool bOverrideWeaponAttachScale = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon Visual|Attach", meta = (EditCondition = "bOverrideWeaponAttachScale"))
	FVector WeaponAttachScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK")
	bool bEnableLeftHandWeaponIK = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK", meta = (ClampMin = "0", ClampMax = "1"))
	float LeftHandWeaponIKAlpha = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK")
	FVector LeftHandIKEffectorLocationOffset_CS = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Elbow")
	FVector LeftElbowJointTarget_BoneSpace = FVector(0.f, -80.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Elbow")
	bool bUseDynamicLeftElbowTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Elbow")
	FName LeftElbowReferenceBoneName = FName("lowerarm_l");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Elbow")
	bool bLeftElbowOffsetInReferenceBoneSpace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Elbow")
	FVector LeftElbowDynamicOffset = FVector(0.f, -60.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Socket Rotation")
	bool bUseLeftHandIKRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Socket Rotation", meta = (ClampMin = "0", ClampMax = "1"))
	float LeftHandIKRotationAlpha = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Socket Rotation")
	FRotator LeftHandIKRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Rotation Distribution")
	bool bUseLeftForearmAdditiveRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Rotation Distribution")
	FRotator LeftForearmAdditiveRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Rotation Distribution", meta = (ClampMin = "0", ClampMax = "1"))
	float LeftForearmAdditiveRotationAlpha = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Rotation Distribution")
	bool bUseLeftHandAdditiveRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Rotation Distribution")
	FRotator LeftHandAdditiveRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Rotation Distribution", meta = (ClampMin = "0", ClampMax = "1"))
	float LeftHandAdditiveRotationAlpha = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Grip")
	bool bEnableWeaponGripPose = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Grip", meta = (ClampMin = "0", ClampMax = "1"))
	float LeftHandGripAlpha = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Weapon IK|Grip", meta = (ClampMin = "0", ClampMax = "1"))
	float RightHandGripAlpha = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Faction")
	ESquadFaction Faction = ESquadFaction::Enemy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Faction")
	float AccuracyMultiplier = 0.65f;

	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamId) override { TeamId = InTeamId; }

	UPROPERTY(BlueprintReadOnly, Category = "Soldier|Cover")
	bool bIsInCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier|Cover")
	bool bIsCrouchingInCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier|Cover")
	FCoverPoint CurrentCoverPoint;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier|Cover")
	ESoldierCoverSide CoverSide = ESoldierCoverSide::Right;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier|Cover")
	ESoldierCoverPeekType CoverPeekType = ESoldierCoverPeekType::Side;

	UFUNCTION(BlueprintCallable, Category = "Soldier|Cover")
	void SetCoverState(bool bInCover, bool bCrouching);

	UFUNCTION(BlueprintCallable, Category = "Soldier|Cover")
	void SetCurrentCover(const FCoverPoint& Cover);

	UFUNCTION(BlueprintCallable, Category = "Soldier|Cover")
	void ClearCover();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier")
	bool IsAlive() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier")
	bool IsEnemy() const { return Faction == ESquadFaction::Enemy; }

	UFUNCTION(BlueprintCallable, Category = "Soldier|Pack")
	void NotifyPackDamage(float DamageAmount, AActor* DamageCauser, AController* InstigatorController, TSubclassOf<UDamageType> DamageTypeClass);

	UFUNCTION(BlueprintCallable, Category = "Soldier|Pack")
	void SetUsePackPresentationMode(bool bEnabled);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier")
	bool IsAlly() const { return Faction == ESquadFaction::Ally; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Animation")
	float GetGroundSpeed() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Animation")
	bool IsMovingInCover(float SpeedThreshold = 20.f) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Cover")
	bool IsCurrentCoverLow() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Cover")
	bool IsCoverPeekOver() const { return CoverPeekType == ESoldierCoverPeekType::Over; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Animation")
	UAnimMontage* SelectFireMontage() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Animation")
	UAnimMontage* SelectReloadMontage() const;

	UFUNCTION(BlueprintCallable, Category = "Soldier|Weapon Visual")
	void SetEquippedWeaponMesh(UMeshComponent* InWeaponMesh);

	UFUNCTION(BlueprintCallable, Category = "Soldier|Weapon Visual")
	void ApplyEquippedWeaponAttachment();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Weapon Visual")
	bool GetEquippedWeaponSocketTransform(FName SocketName, FTransform& OutTransform) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Weapon Visual")
	bool GetEquippedWeaponSocketTransformInMeshSpace(FName SocketName, FTransform& OutTransform) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Weapon Visual")
	bool GetLeftHandIKWorldTransform(FTransform& OutTransform) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Weapon Visual")
	bool GetLeftHandIKMeshSpaceTransform(FTransform& OutTransform) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Weapon Visual")
	bool GetLeftHandIKTransformRelativeToRightHand(FTransform& OutTransform) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Weapon Visual")
	bool GetMuzzleWorldTransform(FTransform& OutTransform) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Weapon IK")
	FWeaponIKRuntimeData GetWeaponIKRuntimeData() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Animation")
	FSoldierAnimRuntimeData GetSoldierAnimRuntimeData(float CoverMoveSpeedThreshold = 20.f, float CoverAdjustDirectionThreshold = 0.35f, float CombatThreatConfidence = 0.25f) const;

	UFUNCTION(BlueprintCallable, Category = "Soldier|Animation")
	void PlayFireAnimation();

	UFUNCTION(BlueprintCallable, Category = "Soldier|Animation")
	void PlayReloadAnimation();

	UPROPERTY(BlueprintReadWrite, Category = "Soldier|Mission")
	FVector GlobalGoalLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Soldier|Mission")
	bool bHasGlobalGoal = false;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Pack")
	bool IsUsingPackPresentationMode() const { return bUsePackPresentationMode; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Pack")
	bool HasEquippedWeaponVisual() const { return EquippedWeaponMesh != nullptr; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Pack")
	FName GetWeaponAttachSocketName() const { return WeaponAttachSocketName; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Pack")
	FName GetWeaponLeftHandIKSocketName() const { return WeaponLeftHandIKSocketName; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Pack")
	FName GetWeaponMuzzleSocketName() const { return WeaponMuzzleSocketName; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Pack")
	UMeshComponent* GetEquippedWeaponVisual() const { return EquippedWeaponMesh; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Animation")
	bool IsCombatReadyForPresentation(float CombatThreatConfidence = 0.25f) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Animation")
	ESquadCombatPresentationState GetSimplePresentationState(float SpeedThreshold = 10.f, float CombatThreatConfidence = 0.25f) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Animation")
	bool IsAimingForPresentation() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Animation")
	bool IsFiringForPresentation() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier|Animation")
	bool IsMovingForPresentation(float SpeedThreshold = 10.f) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Fire")
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Fire")
	TObjectPtr<UAnimMontage> FireMontage_Crouch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Fire")
	TObjectPtr<UAnimMontage> FireMontage_CoverHighRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Fire")
	TObjectPtr<UAnimMontage> FireMontage_CoverHighLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Fire")
	TObjectPtr<UAnimMontage> FireMontage_CoverLowRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Fire")
	TObjectPtr<UAnimMontage> FireMontage_CoverLowLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Fire")
	TObjectPtr<UAnimMontage> FireMontage_CoverLowOver;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Reload")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Reload")
	TObjectPtr<UAnimMontage> ReloadMontage_Crouch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Reload")
	TObjectPtr<UAnimMontage> ReloadMontage_CoverHighRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Reload")
	TObjectPtr<UAnimMontage> ReloadMontage_CoverHighLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Reload")
	TObjectPtr<UAnimMontage> ReloadMontage_CoverLowRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Reload")
	TObjectPtr<UAnimMontage> ReloadMontage_CoverLowLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation|Reload")
	TObjectPtr<UAnimMontage> ReloadMontage_CoverLowOver;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier|Animation")
	bool bWantsToFire = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier|Animation")
	float LastFireAnimTime = -1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation")
	TArray<TObjectPtr<UAnimMontage>> HitReactMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation")
	TArray<TObjectPtr<UAnimMontage>> DeathMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation")
	float DeathMontageToRagdollDelay = 2.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION()
	void OnHealthDied(AActor* OwnerActor);

	virtual void HandleDeath();

	void RegisterWithSystems();
	void UnregisterFromSystems();

	FGenericTeamId TeamId;
};
