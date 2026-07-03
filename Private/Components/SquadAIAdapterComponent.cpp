#include "Components/SquadAIAdapterComponent.h"

#include "Actors/SquadAIAutoSetupManager.h"
#include "Characters/SoldierCharacter.h"
#include "Components/AimComponent.h"
#include "Components/ActorComponent.h"
#include "Components/MeshComponent.h"
#include "Components/WeaponComponent.h"
#include "InputCoreTypes.h"
#include "UObject/UnrealType.h"

#define SQUADAI_VERBOSE_DEBUG_LOG(Format, ...) \
	do { if (ASquadAIAutoSetupManager::AreVerboseDebugLogsEnabled()) { UE_LOG(LogTemp, Warning, Format, ##__VA_ARGS__); } } while (false)

namespace SquadAIAdapterPackBridge
{
	static UObject* FindPackWeaponTarget(AActor* OwnerActor)
	{
		if (!OwnerActor)
		{
			return nullptr;
		}

		TArray<UActorComponent*> Components;
		OwnerActor->GetComponents(Components);

		for (UActorComponent* Component : Components)
		{
			if (Component && (Component->ComponentHasTag(TEXT("TPWCS_BPCWeapon")) || Component->ComponentHasTag(TEXT("TPWCS_Weapon"))))
			{
				return Component;
			}
		}

		for (UActorComponent* Component : Components)
		{
			if (!Component)
			{
				continue;
			}

			const FString ObjectName = Component->GetName();
			const FString ClassName = Component->GetClass() ? Component->GetClass()->GetName() : FString();
			if (ObjectName.Contains(TEXT("BPC_WeaponCharacter")) || ClassName.Contains(TEXT("BPC_WeaponCharacter")) || ClassName.Contains(TEXT("WeaponCharacterAI")))
			{
				return Component;
			}
		}

		return nullptr;
	}

	static bool IsSupportedInputParam(const FProperty* Property)
	{
		if (!Property)
		{
			return false;
		}

		if (CastField<FBoolProperty>(Property))
		{
			return true;
		}

		if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			return StructProp->Struct == TBaseStructure<FVector>::Get();
		}

		return false;
	}

	static void SetParam(void* ParamsBuffer, FProperty* Property, const FVector* VectorValue, const bool* BoolValue)
	{
		if (!ParamsBuffer || !Property)
		{
			return;
		}

		if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			if (VectorValue && StructProp->Struct == TBaseStructure<FVector>::Get())
			{
				*StructProp->ContainerPtrToValuePtr<FVector>(ParamsBuffer) = *VectorValue;
			}
			return;
		}

		if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			if (BoolValue)
			{
				BoolProp->SetPropertyValue_InContainer(ParamsBuffer, *BoolValue);
			}
		}
	}

	static bool Invoke(UObject* Target, FName FunctionName, const FVector* AimLocation = nullptr, const bool* BoolValue = nullptr)
	{
		if (!Target || FunctionName.IsNone())
		{
			return false;
		}

		UFunction* Function = Target->FindFunction(FunctionName);
		if (!Function)
		{
			return false;
		}

		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			const FProperty* Property = *It;
			if (!Property || Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
			{
				continue;
			}
			if (!IsSupportedInputParam(Property))
			{
				return false;
			}
		}

		TArray<uint8> Params;
		Params.SetNumZeroed(Function->ParmsSize);
		void* ParamsBuffer = Params.Num() > 0 ? Params.GetData() : nullptr;

		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			FProperty* Property = *It;
			if (!Property || Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
			{
				continue;
			}
			SetParam(ParamsBuffer, Property, AimLocation, BoolValue);
		}

		Target->ProcessEvent(Function, ParamsBuffer);
		return true;
	}

	static bool SetVector(UObject* Target, const TArray<FName>& Names, const FVector& Value)
	{
		if (!Target)
		{
			return false;
		}

		bool bAny = false;
		for (const FName Name : Names)
		{
			if (FStructProperty* Prop = FindFProperty<FStructProperty>(Target->GetClass(), Name))
			{
				if (Prop->Struct == TBaseStructure<FVector>::Get())
				{
					*Prop->ContainerPtrToValuePtr<FVector>(Target) = Value;
					bAny = true;
				}
			}
		}
		return bAny;
	}

	static bool SetObject(UObject* Target, const TArray<FName>& Names, UObject* Value)
	{
		if (!Target || !Value)
		{
			return false;
		}

		bool bAny = false;
		for (const FName Name : Names)
		{
			if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(Target->GetClass(), Name))
			{
				if (!Prop->PropertyClass || Value->IsA(Prop->PropertyClass))
				{
					Prop->SetObjectPropertyValue_InContainer(Target, Value);
					bAny = true;
				}
			}
		}
		return bAny;
	}

	static bool SetBool(UObject* Target, const TArray<FName>& Names, bool bValue)
	{
		if (!Target)
		{
			return false;
		}

		bool bAny = false;
		for (const FName Name : Names)
		{
			if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(Target->GetClass(), Name))
			{
				Prop->SetPropertyValue_InContainer(Target, bValue);
				bAny = true;
			}
		}
		return bAny;
	}

	static bool SetKey(UObject* Target, const TArray<FName>& Names, const FKey& Value)
	{
		if (!Target)
		{
			return false;
		}

		bool bAny = false;
		for (const FName Name : Names)
		{
			if (FStructProperty* Prop = FindFProperty<FStructProperty>(Target->GetClass(), Name))
			{
				if (Prop->Struct == FKey::StaticStruct())
				{
					*Prop->ContainerPtrToValuePtr<FKey>(Target) = Value;
					bAny = true;
				}
			}
		}
		return bAny;
	}

	static bool SetAimingStateProperty(UObject* Target, bool bAiming)
	{
		if (!Target)
		{
			return false;
		}

		bool bAny = false;
		for (const FName StateName : {
			FName(TEXT("aimingState")),
			FName(TEXT("AimingState")),
			FName(TEXT("CurrentAimingState")),
			FName(TEXT("aimingMode")),
			FName(TEXT("AimingMode"))
		})
		{
			if (FByteProperty* ByteProp = FindFProperty<FByteProperty>(Target->GetClass(), StateName))
			{
				ByteProp->SetPropertyValue_InContainer(Target, bAiming ? 1 : 0);
				bAny = true;
				break;
			}
			if (FNameProperty* NameProp = FindFProperty<FNameProperty>(Target->GetClass(), StateName))
			{
				NameProp->SetPropertyValue_InContainer(Target, bAiming ? FName(TEXT("Aiming")) : FName(TEXT("NotAiming")));
				bAny = true;
				break;
			}
			if (FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(Target->GetClass(), StateName))
			{
				BoolProp->SetPropertyValue_InContainer(Target, bAiming);
				bAny = true;
				break;
			}
		}
		return bAny;
	}

	static bool SyncAimData(UObject* Target, const FVector& AimLocation, AActor* AimActor)
	{
		if (!Target)
		{
			return false;
		}

		const bool bSetAimLocation = SetVector(Target,
		{
			FName(TEXT("traceEndPoint")),
			FName(TEXT("TraceEndPoint")),
			FName(TEXT("TraceEndLoc")),
			FName(TEXT("AimingLocation")),
			FName(TEXT("aimingLocation")),
			FName(TEXT("AimLocation")),
			FName(TEXT("TargetLocation"))
		}, AimLocation);

		const bool bSetAimedActor = SetObject(Target,
		{
			FName(TEXT("aimedActor")),
			FName(TEXT("AimedActor")),
			FName(TEXT("TargetActor")),
			FName(TEXT("targetActor"))
		}, AimActor);

		const bool bSetIK = SetBool(Target,
		{
			FName(TEXT("bLeftHandIK?")),
			FName(TEXT("bLeftHandIK")),
			FName(TEXT("LeftHandIK")),
			FName(TEXT("UseLeftHandIK")),
			FName(TEXT("bUseLeftHandIK"))
		}, true);

		const bool bSetAimState = SetAimingStateProperty(Target, true);

		return bSetAimLocation || bSetAimedActor || bSetIK || bSetAimState;
	}

	static bool StartAim(UObject* Target, const FVector& AimLocation, AActor* AimActor)
	{
		const bool bSynced = SyncAimData(Target, AimLocation, AimActor);
		SetAimingStateProperty(Target, true);
		// Call only once per aim start from the adapter, not every tick.
		const bool bCalled = Invoke(Target, TEXT("CII_StartAiming"), &AimLocation, nullptr);
		if (bCalled)
		{
			SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIAdapterPackBridge] CII_StartAiming %s"), *GetNameSafe(Target));
		}
		return bSynced || bCalled;
	}

	static bool StopAim(UObject* Target)
	{
		if (!Target)
		{
			return false;
		}
		SetAimingStateProperty(Target, false);
		return Invoke(Target, TEXT("CII_StopAiming"));
	}

	static bool InvokePackKeyFunction(UObject* Target, FName FunctionName, const FKey& KeyValue)
	{
		if (!Target || FunctionName.IsNone())
		{
			return false;
		}

		UFunction* Function = Target->FindFunction(FunctionName);
		if (!Function)
		{
			return false;
		}

		TArray<uint8> Params;
		Params.SetNumZeroed(Function->ParmsSize);
		void* ParamsBuffer = Params.Num() > 0 ? Params.GetData() : nullptr;

		bool bFoundKeyParam = false;
		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			FProperty* Property = *It;
			if (!Property || Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
			{
				continue;
			}
			if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
			{
				if (StructProp->Struct == FKey::StaticStruct())
				{
					if (ParamsBuffer)
					{
						*StructProp->ContainerPtrToValuePtr<FKey>(ParamsBuffer) = KeyValue;
					}
					bFoundKeyParam = true;
				}
			}
		}

		Target->ProcessEvent(Function, ParamsBuffer);
		return true;
	}

	static bool StartVisualFire(UObject* Target, const FVector& AimLocation, AActor* AimActor)
	{
		if (!Target)
		{
			return false;
		}

		// Step 0: Ensure aiming state is active. CII_StartFiring checks aimingState
		// before playing fire animations/montages. Without this, the BP silently ignores fire.
		Invoke(Target, TEXT("CII_StartAiming"));
		SetAimingStateProperty(Target, true);

		// Step 1: Sync aim data so pack weapon knows where to shoot
		SyncAimData(Target, AimLocation, AimActor);
		SetBool(Target, { FName(TEXT("bIsShooting?")), FName(TEXT("bIsShooting")), FName(TEXT("IsShooting")) }, true);
		SetKey(Target, { FName(TEXT("firingInputKey")), FName(TEXT("FiringInputKey")) }, EKeys::LeftMouseButton);

		bool bAny = false;
		if (Invoke(Target, TEXT("Char_OpenFiringGate")))
		{
			bAny = true;
		}
		if (InvokePackKeyFunction(Target, TEXT("Plr_SetFiringInputKey"), EKeys::LeftMouseButton))
		{
			bAny = true;
			SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIAdapterPackBridge] Plr_SetFiringInputKey(LeftMouseButton) called on %s"), *GetNameSafe(Target));
		}
		if (Invoke(Target, TEXT("CII_StartFiring")))
		{
			bAny = true;
			SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIAdapterPackBridge] CII_StartFiring visual state %s"), *GetNameSafe(Target));
		}
		return bAny;
	}

	static bool StopVisualFire(UObject* Target)
	{
		if (!Target)
		{
			return false;
		}
		SetBool(Target, { FName(TEXT("bIsShooting?")), FName(TEXT("bIsShooting")), FName(TEXT("IsShooting")) }, false);
		SetKey(Target, { FName(TEXT("firingInputKey")), FName(TEXT("FiringInputKey")) }, FKey());
		bool bAny = false;
		if (InvokePackKeyFunction(Target, TEXT("Plr_SetFiringInputKey"), FKey()))
		{
			bAny = true;
			SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIAdapterPackBridge] Plr_SetFiringInputKey(None) called on %s"), *GetNameSafe(Target));
		}
		if (Invoke(Target, TEXT("CII_StopFiring"))) bAny = true;
		if (Invoke(Target, TEXT("Char_CloseFiringGate"))) bAny = true;
		return bAny;
	}

	static bool IsPackFiring(UObject* Target)
	{
		if (!Target)
		{
			return false;
		}
		const FName BoolNames[] = { FName(TEXT("bIsShooting?")), FName(TEXT("bIsShooting")), FName(TEXT("IsShooting")) };
		for (const FName Name : BoolNames)
		{
			if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(Target->GetClass(), Name))
			{
				if (Prop->GetPropertyValue_InContainer(Target))
				{
					return true;
				}
			}
		}
		return false;
	}
}

USquadAIAdapterComponent::USquadAIAdapterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USquadAIAdapterComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeForOwner();
}

void USquadAIAdapterComponent::InitializeForOwner()
{
	SoldierOwner = Cast<ASoldierCharacter>(GetOwner());
	AimComponent = SoldierOwner ? SoldierOwner->AimComp : nullptr;
	WeaponComponent = SoldierOwner ? SoldierOwner->WeaponComp : nullptr;
	bPackSafeMode = SoldierOwner ? SoldierOwner->IsUsingPackPresentationMode() : bPackSafeMode;
}

void USquadAIAdapterComponent::SetMoveCommand(const FSquadAIMoveCommand& NewCommand)
{
	MoveCommand = NewCommand;
}

void USquadAIAdapterComponent::ClearMoveCommand()
{
	MoveCommand = FSquadAIMoveCommand();
}

void USquadAIAdapterComponent::SetAimCommand(const FSquadAIAimCommand& NewCommand)
{
	AimCommand = NewCommand;

	if (bPackSafeMode || (SoldierOwner && SoldierOwner->IsUsingPackPresentationMode()))
	{
		UObject* PackWeaponTarget = SquadAIAdapterPackBridge::FindPackWeaponTarget(GetOwner());
		if (AimCommand.IsValid() && PackWeaponTarget)
		{
			const bool bTargetChanged = LastPackAimTargetObject.Get() != AimCommand.TargetActor;
			const bool bLocationChanged = FVector::DistSquared(LastPackAimLocation, AimCommand.AimLocation) > FMath::Square(50.f);
			SquadAIAdapterPackBridge::SyncAimData(PackWeaponTarget, AimCommand.AimLocation, AimCommand.TargetActor);

			if (!bPackAimingActive || bTargetChanged)
			{
				SquadAIAdapterPackBridge::StartAim(PackWeaponTarget, AimCommand.AimLocation, AimCommand.TargetActor);
				bPackAimingActive = true;
			}

			LastPackAimTargetObject = AimCommand.TargetActor;
			LastPackAimLocation = AimCommand.AimLocation;

			if (bLocationChanged || bTargetChanged)
			{
				SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIAdapterAimDebug] Owner=%s TargetActor=%s Aim=%s"), *GetNameSafe(GetOwner()), *GetNameSafe(AimCommand.TargetActor), *AimCommand.AimLocation.ToString());
			}
		}
		else if (PackWeaponTarget && bPackAimingActive)
		{
			SquadAIAdapterPackBridge::StopAim(PackWeaponTarget);
			bPackAimingActive = false;
			LastPackAimTargetObject = nullptr;
			LastPackAimLocation = FVector::ZeroVector;
		}
		return;
	}

	if (AimComponent)
	{
		if (AimCommand.IsValid()) AimComponent->SetAimTarget(AimCommand.AimLocation);
		else AimComponent->ClearAimTarget();
	}
}

void USquadAIAdapterComponent::ClearAimCommand()
{
	AimCommand = FSquadAIAimCommand();

	if (bPackSafeMode || (SoldierOwner && SoldierOwner->IsUsingPackPresentationMode()))
	{
		if (UObject* PackWeaponTarget = SquadAIAdapterPackBridge::FindPackWeaponTarget(GetOwner()))
		{
			if (bPackFiringActive)
			{
				SquadAIAdapterPackBridge::StopVisualFire(PackWeaponTarget);
				bPackFiringActive = false;
			}
			if (bPackAimingActive)
			{
				SquadAIAdapterPackBridge::StopAim(PackWeaponTarget);
				bPackAimingActive = false;
			}
		}
		LastPackAimTargetObject = nullptr;
		LastPackAimLocation = FVector::ZeroVector;
		return;
	}

	if (AimComponent)
	{
		AimComponent->ClearAimTarget();
	}
}

bool USquadAIAdapterComponent::TryStartBurstAtAimCommand()
{
	if (!AimCommand.IsValid())
	{
		return false;
	}

	if (bPackSafeMode || (SoldierOwner && SoldierOwner->IsUsingPackPresentationMode()))
	{
		UObject* PackWeaponTarget = SquadAIAdapterPackBridge::FindPackWeaponTarget(GetOwner());
		bool bPackVisualStarted = false;
		if (!PackWeaponTarget)
		{
			SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIAdapterPackBridge] WARNING: Pack weapon not found on %s — cannot fire visually"), *GetNameSafe(GetOwner()));
		}
		else
		{
			bPackVisualStarted = SquadAIAdapterPackBridge::StartVisualFire(PackWeaponTarget, AimCommand.AimLocation, AimCommand.TargetActor);
			bPackFiringActive = true;
			if (!bPackVisualStarted)
			{
				SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIAdapterPackBridge] WARNING: Pack visual fire failed on %s"), *GetNameSafe(GetOwner()));
			}
		}

		bool bGameplayFireStarted = false;
		if (!PackWeaponTarget && WeaponComponent)
		{
			WeaponComponent->StartBurst(AimCommand.AimLocation);
			bGameplayFireStarted = true;
		}

		SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIAdapterTryFireDebug] Owner=%s AimActor=%s PackVisual=%d GameplayFire=%d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(AimCommand.TargetActor),
			bPackVisualStarted ? 1 : 0,
			bGameplayFireStarted ? 1 : 0);
		return PackWeaponTarget != nullptr || WeaponComponent != nullptr;
	}

	if (!WeaponComponent)
	{
		return false;
	}

	WeaponComponent->StartBurst(AimCommand.AimLocation);
	return true;
}

bool USquadAIAdapterComponent::IsBurstFiring() const
{
	if (bPackSafeMode || (SoldierOwner && SoldierOwner->IsUsingPackPresentationMode()))
	{
		UObject* PackWeaponTarget = SquadAIAdapterPackBridge::FindPackWeaponTarget(GetOwner());
		return (PackWeaponTarget && SquadAIAdapterPackBridge::IsPackFiring(PackWeaponTarget)) || bPackFiringActive;
	}

	return WeaponComponent && WeaponComponent->CurrentState == EWeaponState::Firing;
}

void USquadAIAdapterComponent::StopBurstAndClearAim()
{
	if (bPackSafeMode || (SoldierOwner && SoldierOwner->IsUsingPackPresentationMode()))
	{
		if (UObject* PackWeaponTarget = SquadAIAdapterPackBridge::FindPackWeaponTarget(GetOwner()))
		{
			if (bPackFiringActive)
			{
				SquadAIAdapterPackBridge::StopVisualFire(PackWeaponTarget);
			}
			if (bPackAimingActive)
			{
				SquadAIAdapterPackBridge::StopAim(PackWeaponTarget);
			}
		}
		bPackFiringActive = false;
		bPackAimingActive = false;
		LastPackAimTargetObject = nullptr;
		LastPackAimLocation = FVector::ZeroVector;
		AimCommand = FSquadAIAimCommand();
		return;
	}

	if (WeaponComponent)
	{
		WeaponComponent->StopFiring();
	}
	ClearAimCommand();
}

void USquadAIAdapterComponent::EnablePackSafeMode(bool bEnabled)
{
	if (!SoldierOwner)
	{
		InitializeForOwner();
	}

	if (SoldierOwner)
	{
		SoldierOwner->SetUsePackPresentationMode(bEnabled);
	}
	bPackSafeMode = bEnabled;

	if (AimComponent)
	{
		AimComponent->bAffectControllerRotation = false;
	}
}
