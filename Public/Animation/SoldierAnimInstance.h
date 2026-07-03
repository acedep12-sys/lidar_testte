// SoldierAnimInstance.h — C++ base AnimInstance for SquadAI soldiers
// Computes common locomotion/combat/cover/weapon-IK variables so AnimBPs do not need
// large EventGraph math networks.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Characters/SoldierCharacter.h"
#include "SoldierAnimInstance.generated.h"

class ASoldierAIController;

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API USoldierAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// Tuning for generated anim data.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier Anim|Tuning")
	float CoverMoveSpeedThreshold = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier Anim|Tuning")
	float CoverAdjustDirectionThreshold = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier Anim|Tuning")
	float CombatThreatConfidence = 0.25f;

	// Cached owner references.
	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Owner")
	TObjectPtr<ASoldierCharacter> SoldierOwner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Owner")
	TObjectPtr<ASoldierAIController> SoldierAI = nullptr;

	// Core locomotion.
	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Locomotion")
	float AnimSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Locomotion")
	float AnimDirection = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Locomotion")
	bool bAnimIsCrouched = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|State")
	bool bAnimIsAlive = true;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|State")
	bool bAnimCombatReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|State")
	bool bAnimWantsToFire = false;

	// Cover.
	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bAnimIsInCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bAnimIsCrouchingInCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bAnimIsPeekingFromCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bAnimCoverIsLow = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bAnimCoverSideIsLeft = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bAnimCoverPeekIsOver = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bAnimIsMovingInCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	bool bAnimIsAdjustingCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Cover")
	float AnimCoverMoveDirection = 0.f;

	// Aim.
	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Aim")
	float AnimAimYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Aim")
	float AnimAimPitch = 0.f;

	// Weapon IK. These are ready to feed into TwoBoneIK/TransformModifyBone nodes.
	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	bool bAnimEnableLeftHandIK = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	float AnimLeftHandIKAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	FVector AnimLeftHandIKLocationCS = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	FRotator AnimLeftHandIKRotationCS = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	FVector AnimLeftElbowJointTargetCS = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	FVector AnimLeftElbowJointTargetBS = FVector(0.f, -80.f, 0.f);

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	bool bAnimUseDynamicElbowTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	bool bAnimUseLeftHandRotation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	float AnimLeftHandRotationAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	bool bAnimUseLeftForearmAdditiveRotation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	FRotator AnimLeftForearmAdditiveRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	float AnimLeftForearmAdditiveRotationAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	bool bAnimUseLeftHandAdditiveRotation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	FRotator AnimLeftHandAdditiveRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Weapon IK")
	float AnimLeftHandAdditiveRotationAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Grip")
	bool bAnimUseWeaponGripPose = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Grip")
	float AnimLeftHandGripAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Anim|Grip")
	float AnimRightHandGripAlpha = 0.f;

protected:
	void RefreshOwnerRefs();
	void ApplyRuntimeData(const FSoldierAnimRuntimeData& Data);
};
