// SoldierCharacter.cpp — Base soldier with components, faction, cover state
#include "Characters/SoldierCharacter.h"
#include "AI/SoldierAIController.h"
#include "Components/AimComponent.h"
#include "Components/WeaponComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SquadAIAdapterComponent.h"
#include "Characters/SquadLeaderTypes.h"
#include "Performance/AISignificanceManager.h"
#include "Performance/SoldierRegistrySubsystem.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Damage.h"
#include "Components/CapsuleComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"
#include "SquadAITuning.h"
#include "Engine/World.h"

ASoldierCharacter::ASoldierCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = ASoldierAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	AimComp = CreateDefaultSubobject<UAimComponent>(TEXT("AimComp"));
	WeaponComp = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComp"));
	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	AIAdapterComp = CreateDefaultSubobject<USquadAIAdapterComponent>(TEXT("SquadAIAdapter"));

	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->bOrientRotationToMovement = true;
		CMC->bUseControllerDesiredRotation = true;
		CMC->RotationRate = FRotator(0.f, 720.f, 0.f);
		CMC->bRequestedMoveUseAcceleration = true;
		CMC->MaxWalkSpeed = 400.f;
		CMC->MaxStepHeight = 40.f;
		CMC->GetNavAgentPropertiesRef().bCanCrouch = true;
		CMC->SetCrouchedHalfHeight(44.f);
	}

	PerceptionStimuli = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuli"));
	PerceptionStimuli->bAutoRegister = true;
	PerceptionStimuli->RegisterForSense(UAISense_Sight::StaticClass());
	PerceptionStimuli->RegisterForSense(UAISense_Hearing::StaticClass());
	PerceptionStimuli->RegisterForSense(UAISense_Damage::StaticClass());

	TeamId = FGenericTeamId(1);
}

void ASoldierCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->GetNavAgentPropertiesRef().bCanCrouch = true;
		CMC->SetCrouchedHalfHeight(44.f);
	}

	TeamId = (Faction == ESquadFaction::Enemy) ? FGenericTeamId(1) : FGenericTeamId(0);

	if (AIAdapterComp)
	{
		AIAdapterComp->InitializeForOwner();
		AIAdapterComp->EnablePackSafeMode(bUsePackPresentationMode);
	}

	if (ASoldierAIController* AIC = Cast<ASoldierAIController>(GetController()))
	{
		AIC->SetGenericTeamId(TeamId);
	}

	RegisterWithSystems();
	if (WeaponComp) WeaponComp->AccuracyMultiplier = AccuracyMultiplier;
	if (HealthComp) HealthComp->OnDeath.AddDynamic(this, &ASoldierCharacter::OnHealthDied);
}

void ASoldierCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterFromSystems();
	Super::EndPlay(EndPlayReason);
}

float ASoldierCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (HealthComp && HealthComp->IsAlive())
	{
		HealthComp->ApplyDamage(DamageAmount, DamageCauser);
		if (HealthComp->IsAlive())
		{
			UAnimMontage* ReactMontage = HitReactMontage;
			if (HitReactMontages.Num() > 0)
			{
				ReactMontage = HitReactMontages[FMath::RandRange(0, HitReactMontages.Num() - 1)];
			}
			if (ReactMontage)
			{
				if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
				{
					if (!AnimInst->Montage_IsPlaying(ReactMontage))
					{
						AnimInst->Montage_Play(ReactMontage, 1.f);
					}
				}
			}
		}
		UAISense_Damage::ReportDamageEvent(GetWorld(), this, DamageCauser, DamageAmount, GetActorLocation(), DamageCauser ? DamageCauser->GetActorLocation() : GetActorLocation());
	}
	return DamageAmount;
}

void ASoldierCharacter::NotifyPackDamage(float DamageAmount, AActor* DamageCauser, AController* InstigatorController, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (!HealthComp || !HealthComp->IsAlive())
	{
		return;
	}

	HealthComp->ApplyExternalDamage(DamageAmount, DamageCauser, InstigatorController, DamageTypeClass);
	UAISense_Damage::ReportDamageEvent(GetWorld(), this, DamageCauser, DamageAmount, GetActorLocation(), DamageCauser ? DamageCauser->GetActorLocation() : GetActorLocation());
}

void ASoldierCharacter::SetUsePackPresentationMode(bool bEnabled)
{
	bUsePackPresentationMode = bEnabled;
	if (bUsePackPresentationMode)
	{
		bAutoAttachEquippedWeaponToMesh = false;
		bEnableLeftHandWeaponIK = false;
	}
}

void ASoldierCharacter::OnHealthDied(AActor* OwnerActor)
{
	HandleDeath();
}

void ASoldierCharacter::HandleDeath()
{
	UnregisterFromSystems();
	if (WeaponComp) WeaponComp->StopFiring();
	if (AimComp) AimComp->ClearAimTarget();

	if (ASoldierAIController* AIC = Cast<ASoldierAIController>(GetController()))
	{
		AIC->StopMovement();
		if (AIC->StateTreeComp)
		{
			AIC->StateTreeComp->StopLogic(TEXT("Dead"));
		}
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	float PlayedLength = 0.f;
	UAnimMontage* SelectedDeathMontage = DeathMontage;
	if (DeathMontages.Num() > 0)
	{
		SelectedDeathMontage = DeathMontages[FMath::RandRange(0, DeathMontages.Num() - 1)];
	}

	if (SelectedDeathMontage && GetMesh())
	{
		if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
		{
			PlayedLength = AnimInst->Montage_Play(SelectedDeathMontage, 1.f);
			UE_LOG(LogTemp, Warning, TEXT("DeathAnim: %s Montage_Play(%s) on AnimBP=%s returned %.3f"),
				*GetNameSafe(this), *GetNameSafe(SelectedDeathMontage), *GetNameSafe(AnimInst->GetClass()), PlayedLength);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("DeathAnim: %s has DeathMontage %s but mesh has no AnimInstance."),
				*GetNameSafe(this), *GetNameSafe(SelectedDeathMontage));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DeathAnim: %s has no DeathMontage assigned; using ragdoll fallback."), *GetNameSafe(this));
	}

	if (GetMesh())
	{
		if (PlayedLength > 0.f)
		{
			const float RagdollDelay = FMath::Max(DeathMontageToRagdollDelay, PlayedLength * 0.90f);
			TWeakObjectPtr<ASoldierCharacter> WeakSelf(this);
			FTimerHandle RagdollTimer;
			GetWorldTimerManager().SetTimer(RagdollTimer, [WeakSelf]()
			{
				if (!WeakSelf.IsValid() || !WeakSelf->GetMesh()) return;
				WeakSelf->GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
				WeakSelf->GetMesh()->SetSimulatePhysics(true);
			}, RagdollDelay, false);
		}
		else
		{
			GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
			GetMesh()->SetSimulatePhysics(true);
		}
	}

	DetachFromControllerPendingDestroy();
	SetLifeSpan(10.f);
}

void ASoldierCharacter::RegisterWithSystems()
{
	if (UWorld* World = GetWorld())
	{
		if (UAISignificanceManager* SM = World->GetSubsystem<UAISignificanceManager>()) SM->RegisterSoldier(this);
		if (USoldierRegistrySubsystem* Reg = World->GetSubsystem<USoldierRegistrySubsystem>()) Reg->RegisterSoldier(this);
	}
}

void ASoldierCharacter::UnregisterFromSystems()
{
	if (UWorld* World = GetWorld())
	{
		if (UAISignificanceManager* SM = World->GetSubsystem<UAISignificanceManager>()) SM->UnregisterSoldier(this);
		if (USoldierRegistrySubsystem* Reg = World->GetSubsystem<USoldierRegistrySubsystem>()) Reg->UnregisterSoldier(this);
	}
}

void ASoldierCharacter::SetCoverState(bool bInCover, bool bCrouching)
{
	bIsInCover = bInCover;
	bIsCrouchingInCover = bCrouching;
	if (bCrouching && !bIsCrouched) Crouch(); else if (!bCrouching && bIsCrouched) UnCrouch();
}

void ASoldierCharacter::SetCurrentCover(const FCoverPoint& Cover)
{
	CurrentCoverPoint = Cover;

	if (EnumHasAnyFlags(Cover.AvailableLeans, ELeanSide::Right))
	{
		CoverSide = ESoldierCoverSide::Right;
	}
	else if (EnumHasAnyFlags(Cover.AvailableLeans, ELeanSide::Left))
	{
		CoverSide = ESoldierCoverSide::Left;
	}
	else
	{
		CoverSide = ESoldierCoverSide::Right;
	}

	if (Cover.Height == ECoverHeight::Low && EnumHasAnyFlags(Cover.AvailableLeans, ELeanSide::Over))
	{
		CoverPeekType = ESoldierCoverPeekType::Over;
	}
	else
	{
		CoverPeekType = ESoldierCoverPeekType::Side;
	}
}

void ASoldierCharacter::ClearCover()
{
	bIsInCover = false;
	bIsCrouchingInCover = false;
	if (bIsCrouched) UnCrouch();
}

bool ASoldierCharacter::IsAlive() const { return HealthComp ? HealthComp->IsAlive() : true; }
float ASoldierCharacter::GetHealthPercent() const { return HealthComp ? HealthComp->GetHealthPercent() : 1.f; }

float ASoldierCharacter::GetGroundSpeed() const
{
	return GetVelocity().Size2D();
}

bool ASoldierCharacter::IsMovingInCover(float SpeedThreshold) const
{
	return bIsInCover && GetGroundSpeed() > SpeedThreshold;
}

bool ASoldierCharacter::IsCurrentCoverLow() const
{
	return CurrentCoverPoint.Height == ECoverHeight::Low;
}

UAnimMontage* ASoldierCharacter::SelectFireMontage() const
{
	if (bIsInCover)
	{
		if (IsCurrentCoverLow())
		{
			if (CoverPeekType == ESoldierCoverPeekType::Over && FireMontage_CoverLowOver) return FireMontage_CoverLowOver;
			if (CoverSide == ESoldierCoverSide::Left && FireMontage_CoverLowLeft) return FireMontage_CoverLowLeft;
			if (FireMontage_CoverLowRight) return FireMontage_CoverLowRight;
		}
		else
		{
			if (CoverSide == ESoldierCoverSide::Left && FireMontage_CoverHighLeft) return FireMontage_CoverHighLeft;
			if (FireMontage_CoverHighRight) return FireMontage_CoverHighRight;
		}
	}

	if (bIsCrouched && FireMontage_Crouch) return FireMontage_Crouch;
	return FireMontage;
}

UAnimMontage* ASoldierCharacter::SelectReloadMontage() const
{
	if (bIsInCover)
	{
		if (IsCurrentCoverLow())
		{
			if (CoverPeekType == ESoldierCoverPeekType::Over && ReloadMontage_CoverLowOver) return ReloadMontage_CoverLowOver;
			if (CoverSide == ESoldierCoverSide::Left && ReloadMontage_CoverLowLeft) return ReloadMontage_CoverLowLeft;
			if (ReloadMontage_CoverLowRight) return ReloadMontage_CoverLowRight;
		}
		else
		{
			if (CoverSide == ESoldierCoverSide::Left && ReloadMontage_CoverHighLeft) return ReloadMontage_CoverHighLeft;
			if (ReloadMontage_CoverHighRight) return ReloadMontage_CoverHighRight;
		}
	}

	if (bIsCrouched && ReloadMontage_Crouch) return ReloadMontage_Crouch;
	return ReloadMontage;
}

void ASoldierCharacter::SetEquippedWeaponMesh(UMeshComponent* InWeaponMesh)
{
	EquippedWeaponMesh = InWeaponMesh;
	ApplyEquippedWeaponAttachment();
}

void ASoldierCharacter::ApplyEquippedWeaponAttachment()
{
	if (!EquippedWeaponMesh || !GetMesh() || !bAutoAttachEquippedWeaponToMesh || bUsePackPresentationMode)
	{
		return;
	}

	EquippedWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
	EquippedWeaponMesh->SetRelativeLocation(WeaponAttachLocationOffset);
	EquippedWeaponMesh->SetRelativeRotation(WeaponAttachRotationOffset);
	if (bOverrideWeaponAttachScale)
	{
		EquippedWeaponMesh->SetRelativeScale3D(WeaponAttachScale);
	}
	EquippedWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool ASoldierCharacter::GetEquippedWeaponSocketTransform(FName SocketName, FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;
	if (!EquippedWeaponMesh || SocketName.IsNone()) return false;
	if (!EquippedWeaponMesh->DoesSocketExist(SocketName)) return false;
	OutTransform = EquippedWeaponMesh->GetSocketTransform(SocketName, RTS_World);
	return true;
}

bool ASoldierCharacter::GetEquippedWeaponSocketTransformInMeshSpace(FName SocketName, FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;
	FTransform SocketWorld;
	if (!GetEquippedWeaponSocketTransform(SocketName, SocketWorld)) return false;
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return false;
	OutTransform = SocketWorld.GetRelativeTransform(MeshComp->GetComponentTransform());
	return true;
}

bool ASoldierCharacter::GetLeftHandIKWorldTransform(FTransform& OutTransform) const
{
	return GetEquippedWeaponSocketTransform(WeaponLeftHandIKSocketName, OutTransform);
}

bool ASoldierCharacter::GetLeftHandIKMeshSpaceTransform(FTransform& OutTransform) const
{
	return GetEquippedWeaponSocketTransformInMeshSpace(WeaponLeftHandIKSocketName, OutTransform);
}

bool ASoldierCharacter::GetLeftHandIKTransformRelativeToRightHand(FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;
	FTransform LeftHandWorld;
	if (!GetLeftHandIKWorldTransform(LeftHandWorld)) return false;
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return false;
	FTransform RightHandWorld = MeshComp->GetSocketTransform(FName("hand_r"), RTS_World);
	OutTransform = LeftHandWorld.GetRelativeTransform(RightHandWorld);
	return true;
}

bool ASoldierCharacter::GetMuzzleWorldTransform(FTransform& OutTransform) const
{
	return GetEquippedWeaponSocketTransform(WeaponMuzzleSocketName, OutTransform);
}

FWeaponIKRuntimeData ASoldierCharacter::GetWeaponIKRuntimeData() const
{
	FWeaponIKRuntimeData Data;
	Data.bEnabled = false;
	Data.Alpha = 0.f;
	Data.LeftElbowJointTargetBS = LeftElbowJointTarget_BoneSpace;

	if (!bEnableLeftHandWeaponIK || !IsAlive()) return Data;

	FTransform LeftHandMeshSpace;
	if (!GetLeftHandIKMeshSpaceTransform(LeftHandMeshSpace)) return Data;

	Data.bEnabled = true;
	Data.Alpha = LeftHandWeaponIKAlpha;
	Data.LeftHandEffectorLocationCS = LeftHandMeshSpace.GetLocation() + LeftHandIKEffectorLocationOffset_CS;
	Data.LeftHandEffectorRotationCS = (LeftHandMeshSpace.GetRotation().Rotator() + LeftHandIKRotationOffset).GetNormalized();
	Data.LeftElbowJointTargetBS = LeftElbowJointTarget_BoneSpace;

	if (bUseDynamicLeftElbowTarget)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			const bool bHasRefBone = MeshComp->GetBoneIndex(LeftElbowReferenceBoneName) != INDEX_NONE || MeshComp->DoesSocketExist(LeftElbowReferenceBoneName);
			if (bHasRefBone)
			{
				const FTransform RefCS = MeshComp->GetSocketTransform(LeftElbowReferenceBoneName, RTS_Component);
				Data.LeftElbowJointTargetCS = bLeftElbowOffsetInReferenceBoneSpace ? RefCS.TransformPosition(LeftElbowDynamicOffset) : RefCS.GetLocation() + LeftElbowDynamicOffset;
				Data.bUseDynamicElbowTarget = true;
			}
		}
	}

	Data.bUseLeftHandRotation = bUseLeftHandIKRotation;
	Data.LeftHandRotationAlpha = bUseLeftHandIKRotation ? LeftHandIKRotationAlpha : 0.f;
	Data.bUseLeftForearmAdditiveRotation = bUseLeftForearmAdditiveRotation;
	Data.LeftForearmAdditiveRotation = LeftForearmAdditiveRotationOffset;
	Data.LeftForearmAdditiveRotationAlpha = bUseLeftForearmAdditiveRotation ? LeftForearmAdditiveRotationAlpha : 0.f;
	Data.bUseLeftHandAdditiveRotation = bUseLeftHandAdditiveRotation;
	Data.LeftHandAdditiveRotation = LeftHandAdditiveRotationOffset;
	Data.LeftHandAdditiveRotationAlpha = bUseLeftHandAdditiveRotation ? LeftHandAdditiveRotationAlpha : 0.f;
	Data.bUseWeaponGripPose = bEnableWeaponGripPose;
	Data.LeftHandGripAlpha = bEnableWeaponGripPose ? LeftHandGripAlpha : 0.f;
	Data.RightHandGripAlpha = bEnableWeaponGripPose ? RightHandGripAlpha : 0.f;
	return Data;
}

FSoldierAnimRuntimeData ASoldierCharacter::GetSoldierAnimRuntimeData(float CoverMoveSpeedThreshold, float CoverAdjustDirectionThreshold, float CombatThreatConfidence) const
{
	FSoldierAnimRuntimeData Data;

	const FVector Velocity = GetVelocity();
	const FVector Velocity2D(Velocity.X, Velocity.Y, 0.f);
	Data.Speed = Velocity2D.Size();
	Data.Direction = 0.f;
	if (Data.Speed > KINDA_SMALL_NUMBER)
	{
		const FVector MoveDir = Velocity2D.GetSafeNormal();
		const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
		const FVector Right = GetActorRightVector().GetSafeNormal2D();
		const float ForwardDot = FVector::DotProduct(MoveDir, Forward);
		const float RightDot = FVector::DotProduct(MoveDir, Right);
		Data.Direction = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));
	}

	Data.bIsAlive = IsAlive();
	Data.bIsCrouched = bIsCrouched;
	Data.bWantsToFire = bWantsToFire;
	Data.bIsInCover = bIsInCover;
	Data.bIsCrouchingInCover = bIsCrouchingInCover;
	Data.bIsPeekingFromCover = bIsInCover && !bIsCrouchingInCover;
	Data.bCoverIsLow = IsCurrentCoverLow();
	Data.bCoverSideIsLeft = CoverSide == ESoldierCoverSide::Left;
	Data.bCoverPeekIsOver = CoverPeekType == ESoldierCoverPeekType::Over;
	Data.bIsMovingInCover = bIsInCover && Data.Speed > CoverMoveSpeedThreshold;

	const FVector MoveDir = Data.Speed > KINDA_SMALL_NUMBER ? Velocity2D.GetSafeNormal() : FVector::ZeroVector;
	Data.CoverMoveDirection = Data.Speed > KINDA_SMALL_NUMBER ? FVector::DotProduct(MoveDir, GetActorRightVector().GetSafeNormal2D()) : 0.f;
	Data.bIsAdjustingCover = Data.bIsMovingInCover && FMath::Abs(Data.CoverMoveDirection) < CoverAdjustDirectionThreshold;

	if (bUsePackPresentationMode)
	{
		Data.AimYaw = 0.f;
		Data.AimPitch = 0.f;
	}
	else if (AimComp)
	{
		const FRotator AimOffset = AimComp->GetSpineAimOffset();
		Data.AimYaw = FMath::Clamp(AimOffset.Yaw, -90.f, 90.f);
		Data.AimPitch = FMath::Clamp(AimOffset.Pitch, -90.f, 90.f);
	}

	if (const ASoldierAIController* AIC = Cast<ASoldierAIController>(GetController()))
	{
		Data.bCombatReady = AIC->HasCombatThreat(CombatThreatConfidence);
	}
	else
	{
		Data.bCombatReady = false;
	}

	Data.WeaponIK = GetWeaponIKRuntimeData();
	return Data;
}

void ASoldierCharacter::PlayFireAnimation()
{
	bWantsToFire = true;
	LastFireAnimTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	if (bUsePackPresentationMode)
	{
		return;
	}

	UAnimMontage* MontageToPlay = SelectFireMontage();
	if (!MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireAnim: %s wants to fire but no suitable FireMontage is assigned. InCover=%d Low=%d Side=%s Peek=%s Crouched=%d"),
			*GetNameSafe(this), bIsInCover ? 1 : 0, IsCurrentCoverLow() ? 1 : 0,
			CoverSide == ESoldierCoverSide::Left ? TEXT("Left") : TEXT("Right"),
			CoverPeekType == ESoldierCoverPeekType::Over ? TEXT("Over") : TEXT("Side"),
			bIsCrouched ? 1 : 0);
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireAnim: %s has montage %s but GetMesh() is null."), *GetNameSafe(this), *GetNameSafe(MontageToPlay));
		return;
	}

	UAnimInstance* AnimInst = MeshComp->GetAnimInstance();
	if (!AnimInst)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireAnim: %s has montage %s but mesh %s has no AnimInstance. Check Anim Class on Mesh."),
			*GetNameSafe(this), *GetNameSafe(MontageToPlay), *GetNameSafe(MeshComp->GetSkeletalMeshAsset()));
		return;
	}

	const float PlayedLength = AnimInst->Montage_Play(MontageToPlay, 1.f);
	UE_LOG(LogTemp, Warning, TEXT("FireAnim: %s Montage_Play(%s) on AnimBP=%s returned %.3f. Mesh=%s InCover=%d Low=%d Side=%s Peek=%s"),
		*GetNameSafe(this), *GetNameSafe(MontageToPlay), *GetNameSafe(AnimInst->GetClass()), PlayedLength,
		*GetNameSafe(MeshComp->GetSkeletalMeshAsset()), bIsInCover ? 1 : 0, IsCurrentCoverLow() ? 1 : 0,
		CoverSide == ESoldierCoverSide::Left ? TEXT("Left") : TEXT("Right"),
		CoverPeekType == ESoldierCoverPeekType::Over ? TEXT("Over") : TEXT("Side"));

	if (PlayedLength <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireAnim: Montage did not start. Usually skeleton mismatch, bad slot setup, or montage incompatible with this AnimInstance."));
	}
}

void ASoldierCharacter::PlayReloadAnimation()
{
	UAnimMontage* MontageToPlay = SelectReloadMontage();
	if (!MontageToPlay)
	{
		UE_LOG(LogTemp, Verbose, TEXT("ReloadAnim: %s requested reload but no suitable ReloadMontage is assigned."), *GetNameSafe(this));
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	UAnimInstance* AnimInst = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!AnimInst)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReloadAnim: %s has montage %s but no AnimInstance."), *GetNameSafe(this), *GetNameSafe(MontageToPlay));
		return;
	}

	const float PlayedLength = AnimInst->Montage_Play(MontageToPlay, 1.f);
	UE_LOG(LogTemp, Warning, TEXT("ReloadAnim: %s Montage_Play(%s) returned %.3f InCover=%d Low=%d Side=%s Peek=%s"),
		*GetNameSafe(this), *GetNameSafe(MontageToPlay), PlayedLength,
		bIsInCover ? 1 : 0, IsCurrentCoverLow() ? 1 : 0,
		CoverSide == ESoldierCoverSide::Left ? TEXT("Left") : TEXT("Right"),
		CoverPeekType == ESoldierCoverPeekType::Over ? TEXT("Over") : TEXT("Side"));
}

#include "SoldierCharacter_PresentationHelpers.inc"
