#include "Components/SquadAIPackLoadoutBridgeComponent.h"

#include "Actors/SquadAIAutoSetupManager.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

#define SQUADAI_VERBOSE_DEBUG_LOG(Format, ...) \
    do { if (ASquadAIAutoSetupManager::AreVerboseDebugLogsEnabled()) { UE_LOG(LogTemp, Warning, Format, ##__VA_ARGS__); } } while (false)

namespace SquadAIPackLoadoutBridgeInternal
{
    static bool LooksLikeTPWCSWeaponComponent(const UActorComponent* Component)
    {
        if (!Component)
        {
            return false;
        }

        const FString ComponentName = Component->GetName();
        const FString ClassName = Component->GetClass() ? Component->GetClass()->GetName() : FString();
        return Component->ComponentHasTag(TEXT("TPWCS_BPCWeapon")) ||
            Component->ComponentHasTag(TEXT("TPWCS_Weapon")) ||
            ComponentName.Contains(TEXT("BPC_WeaponCharacter")) ||
            ClassName.Contains(TEXT("BPC_WeaponCharacter")) ||
            ClassName.Contains(TEXT("WeaponCharacterAI"));
    }

    static bool InvokeGiveWeapon(UObject* Target, const FName WeaponID, const int32 Ammo, const bool bEquip)
    {
        if (!Target)
        {
            return false;
        }

        UFunction* GiveWeaponFn = Target->FindFunction(TEXT("WI_GiveWeapon"));
        if (!GiveWeaponFn)
        {
            return false;
        }

        struct FGiveWeaponParams
        {
            FName WeaponID = NAME_None;
            int32 Ammo = 0;
            bool Equip = false;
            bool AddAmmoIfPossible = false;
        } Params;

        Params.WeaponID = WeaponID;
        Params.Ammo = Ammo;
        Params.Equip = bEquip;
        Params.AddAmmoIfPossible = false;
        Target->ProcessEvent(GiveWeaponFn, &Params);
        return true;
    }

    static void TryStopMelee(UObject* Target)
    {
        if (!Target)
        {
            return;
        }

        if (UFunction* StopMeleeFn = Target->FindFunction(TEXT("TPWCS_StopMeleeAttack")))
        {
            Target->ProcessEvent(StopMeleeFn, nullptr);
        }
        else if (UFunction* StopMeleeFn2 = Target->FindFunction(TEXT("WI_StopMeleeAttack")))
        {
            Target->ProcessEvent(StopMeleeFn2, nullptr);
        }
    }
}

USquadAIPackLoadoutBridgeComponent::USquadAIPackLoadoutBridgeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USquadAIPackLoadoutBridgeComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!bAutoApplyOnBeginPlay)
    {
        return;
    }

    if (BeginPlayDelay > 0.f)
    {
        if (UWorld* World = GetWorld())
        {
            FTimerHandle TmpHandle;
            World->GetTimerManager().SetTimer(TmpHandle, this, &USquadAIPackLoadoutBridgeComponent::DelayedApply, BeginPlayDelay, false);
        }
    }
    else
    {
        DelayedApply();
    }
}

void USquadAIPackLoadoutBridgeComponent::DelayedApply()
{
    TryApplyPackLoadout();
}

bool USquadAIPackLoadoutBridgeComponent::TryApplyPackLoadout()
{
    if (bLoadoutApplied || WeaponIDs.Num() == 0)
    {
        return bLoadoutApplied;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return false;
    }

    const int32 ChosenIndex = bChooseRandomWeapon ? FMath::RandRange(0, WeaponIDs.Num() - 1) : 0;
    const FName SelectedWeaponID = WeaponIDs.IsValidIndex(ChosenIndex) ? WeaponIDs[ChosenIndex] : NAME_None;
    if (SelectedWeaponID.IsNone())
    {
        return false;
    }

    bool bApplied = false;
    UObject* AppliedTarget = nullptr;

    // Prefer the TPWCS weapon logic component. Screenshots/docs and prior testing show the real pack flow
    // is usually: get BPC_WeaponCharacterAI / TPWCS_BPCWeapon component -> call WI_GiveWeapon.
    TArray<UActorComponent*> Components;
    OwnerActor->GetComponents(Components);
    for (UActorComponent* Component : Components)
    {
        const bool bLooksLikeWeapon = SquadAIPackLoadoutBridgeInternal::LooksLikeTPWCSWeaponComponent(Component);
        SQUADAI_VERBOSE_DEBUG_LOG(TEXT("[SquadAIPackLoadoutBridgeDebug] Owner=%s ScanningComponent=%s Class=%s LooksLikeTPWCSWeapon=%d Tags=%d"),
            *GetNameSafe(OwnerActor),
            *GetNameSafe(Component),
            Component ? *GetNameSafe(Component->GetClass()) : TEXT("None"),
            bLooksLikeWeapon ? 1 : 0,
            Component ? Component->ComponentTags.Num() : 0);
        if (!bLooksLikeWeapon)
        {
            continue;
        }

        if (SquadAIPackLoadoutBridgeInternal::InvokeGiveWeapon(Component, SelectedWeaponID, Ammo, bEquip))
        {
            bApplied = true;
            AppliedTarget = Component;
            break;
        }
    }

    // Fallback to actor implementation only if the component path was not available.
    if (!bApplied && SquadAIPackLoadoutBridgeInternal::InvokeGiveWeapon(OwnerActor, SelectedWeaponID, Ammo, bEquip))
    {
        bApplied = true;
        AppliedTarget = OwnerActor;
    }

    if (!bApplied)
    {
        UE_LOG(LogTemp, Error, TEXT("[SquadAIPackLoadoutBridge] Could not find callable WI_GiveWeapon entry on TPWCS weapon component or owner for %s."), *GetNameSafe(OwnerActor));
        return false;
    }

    if (bStopMeleeAfterGive)
    {
        SquadAIPackLoadoutBridgeInternal::TryStopMelee(AppliedTarget);
        if (AppliedTarget != OwnerActor)
        {
            SquadAIPackLoadoutBridgeInternal::TryStopMelee(OwnerActor);
        }
    }

    bLoadoutApplied = true;
    UE_LOG(LogTemp, Warning, TEXT("[SquadAIPackLoadoutBridge] Applied weapon %s to %s through %s"), *SelectedWeaponID.ToString(), *GetNameSafe(OwnerActor), *GetNameSafe(AppliedTarget));
    return true;
}
