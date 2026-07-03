// SoldierAnimInstance.cpp
#include "Animation/SoldierAnimInstance.h"
#include "AI/SoldierAIController.h"
#include "GameFramework/Pawn.h"

void USoldierAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	RefreshOwnerRefs();
}

void USoldierAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!SoldierOwner)
	{
		RefreshOwnerRefs();
	}

	if (!SoldierOwner)
	{
		return;
	}

	ApplyRuntimeData(SoldierOwner->GetSoldierAnimRuntimeData(
		CoverMoveSpeedThreshold,
		CoverAdjustDirectionThreshold,
		CombatThreatConfidence));
}

void USoldierAnimInstance::RefreshOwnerRefs()
{
	APawn* PawnOwner = TryGetPawnOwner();
	SoldierOwner = Cast<ASoldierCharacter>(PawnOwner);
	SoldierAI = SoldierOwner ? Cast<ASoldierAIController>(SoldierOwner->GetController()) : nullptr;
}

void USoldierAnimInstance::ApplyRuntimeData(const FSoldierAnimRuntimeData& Data)
{
	AnimSpeed = Data.Speed;
	AnimDirection = Data.Direction;
	bAnimIsAlive = Data.bIsAlive;
	bAnimIsCrouched = Data.bIsCrouched;
	bAnimCombatReady = Data.bCombatReady;
	bAnimWantsToFire = Data.bWantsToFire;

	bAnimIsInCover = Data.bIsInCover;
	bAnimIsCrouchingInCover = Data.bIsCrouchingInCover;
	bAnimIsPeekingFromCover = Data.bIsPeekingFromCover;
	bAnimCoverIsLow = Data.bCoverIsLow;
	bAnimCoverSideIsLeft = Data.bCoverSideIsLeft;
	bAnimCoverPeekIsOver = Data.bCoverPeekIsOver;
	bAnimIsMovingInCover = Data.bIsMovingInCover;
	bAnimIsAdjustingCover = Data.bIsAdjustingCover;
	AnimCoverMoveDirection = Data.CoverMoveDirection;

	AnimAimYaw = Data.AimYaw;
	AnimAimPitch = Data.AimPitch;

	bAnimEnableLeftHandIK = Data.WeaponIK.bEnabled;
	AnimLeftHandIKAlpha = Data.WeaponIK.Alpha;
	AnimLeftHandIKLocationCS = Data.WeaponIK.LeftHandEffectorLocationCS;
	AnimLeftHandIKRotationCS = Data.WeaponIK.LeftHandEffectorRotationCS;
	AnimLeftElbowJointTargetCS = Data.WeaponIK.LeftElbowJointTargetCS;
	AnimLeftElbowJointTargetBS = Data.WeaponIK.LeftElbowJointTargetBS;
	bAnimUseDynamicElbowTarget = Data.WeaponIK.bUseDynamicElbowTarget;
	bAnimUseLeftHandRotation = Data.WeaponIK.bUseLeftHandRotation;
	AnimLeftHandRotationAlpha = Data.WeaponIK.LeftHandRotationAlpha;

	bAnimUseLeftForearmAdditiveRotation = Data.WeaponIK.bUseLeftForearmAdditiveRotation;
	AnimLeftForearmAdditiveRotation = Data.WeaponIK.LeftForearmAdditiveRotation;
	AnimLeftForearmAdditiveRotationAlpha = Data.WeaponIK.LeftForearmAdditiveRotationAlpha;
	bAnimUseLeftHandAdditiveRotation = Data.WeaponIK.bUseLeftHandAdditiveRotation;
	AnimLeftHandAdditiveRotation = Data.WeaponIK.LeftHandAdditiveRotation;
	AnimLeftHandAdditiveRotationAlpha = Data.WeaponIK.LeftHandAdditiveRotationAlpha;

	bAnimUseWeaponGripPose = Data.WeaponIK.bUseWeaponGripPose;
	AnimLeftHandGripAlpha = Data.WeaponIK.LeftHandGripAlpha;
	AnimRightHandGripAlpha = Data.WeaponIK.RightHandGripAlpha;
}
