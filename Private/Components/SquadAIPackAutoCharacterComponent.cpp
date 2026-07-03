#include "Components/SquadAIPackAutoCharacterComponent.h"

#include "BlueprintLibraries/SquadAIPackBridgeLibrary.h"
#include "Characters/AllySoldier.h"
#include "Characters/LeaderCharacter.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

USquadAIPackAutoCharacterComponent::USquadAIPackAutoCharacterComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USquadAIPackAutoCharacterComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoSetupOnBeginPlay)
    {
        TryAutoSetup();
    }
}

bool USquadAIPackAutoCharacterComponent::RunAutoSetup()
{
    ++AttemptCount;

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return false;
    }

    UMeshComponent* WeaponMesh = ResolveWeaponMeshComponent();
    if (!WeaponMesh)
    {
        return false;
    }

    const bool bOwnerIsLeader = bTreatAsLeader || OwnerActor->IsA<ALeaderCharacter>();

    bool bResult = false;
    if (bOwnerIsLeader)
    {
        bResult = USquadAIPackBridgeLibrary::AutoSetupLeaderForPack(OwnerActor, WeaponMesh);
    }
    else if (OwnerActor->IsA<AAllySoldier>())
    {
        bResult = USquadAIPackBridgeLibrary::AutoSetupAllyForPack(OwnerActor, WeaponMesh);
    }
    else
    {
        bResult = USquadAIPackBridgeLibrary::AutoSetupSquadAIPackCharacter(OwnerActor, WeaponMesh, bEnableLeftHandIK);
    }

    bSetupSucceeded = bResult;
    if (bSetupSucceeded)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(RetryTimerHandle);
        }
    }

    return bSetupSucceeded;
}

UMeshComponent* USquadAIPackAutoCharacterComponent::ResolveWeaponMeshComponent() const
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return nullptr;
    }

    if (UMeshComponent* TaggedMesh = FindWeaponMeshByTag(OwnerActor))
    {
        return TaggedMesh;
    }

    if (UMeshComponent* NamedMesh = FindWeaponMeshByName(OwnerActor))
    {
        return NamedMesh;
    }

    return FindFallbackWeaponMesh(OwnerActor);
}

void USquadAIPackAutoCharacterComponent::TryAutoSetup()
{
    if (bSetupSucceeded)
    {
        return;
    }

    if (!RunAutoSetup() && bRetryIfSetupFails)
    {
        ScheduleRetry();
    }
}

void USquadAIPackAutoCharacterComponent::ScheduleRetry()
{
    if (AttemptCount > MaxRetryCount)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    World->GetTimerManager().SetTimer(
        RetryTimerHandle,
        this,
        &USquadAIPackAutoCharacterComponent::TryAutoSetup,
        RetryDelay,
        false);
}

UMeshComponent* USquadAIPackAutoCharacterComponent::FindWeaponMeshByTag(AActor* OwnerActor) const
{
    TArray<UMeshComponent*> MeshComponents;
    OwnerActor->GetComponents<UMeshComponent>(MeshComponents);

    for (UMeshComponent* MeshComponent : MeshComponents)
    {
        if (MeshComponent && MeshComponent->ComponentHasTag(PreferredWeaponMeshTag))
        {
            return MeshComponent;
        }
    }

    return nullptr;
}

UMeshComponent* USquadAIPackAutoCharacterComponent::FindWeaponMeshByName(AActor* OwnerActor) const
{
    TArray<UMeshComponent*> MeshComponents;
    OwnerActor->GetComponents<UMeshComponent>(MeshComponents);

    for (UMeshComponent* MeshComponent : MeshComponents)
    {
        if (MeshComponent && MeshComponent->GetFName() == PreferredWeaponMeshComponentName)
        {
            return MeshComponent;
        }
    }

    return nullptr;
}

UMeshComponent* USquadAIPackAutoCharacterComponent::FindFallbackWeaponMesh(AActor* OwnerActor) const
{
    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    OwnerActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);

    USkeletalMeshComponent* BestCandidate = nullptr;

    for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
    {
        if (!SkeletalMeshComponent)
        {
            continue;
        }

        const FName ComponentName = SkeletalMeshComponent->GetFName();
        if (ComponentName == TEXT("CharacterMesh0") || ComponentName == TEXT("Mesh") || ComponentName == TEXT("Mesh0"))
        {
            continue;
        }

        BestCandidate = SkeletalMeshComponent;

        if (ComponentName == PreferredWeaponMeshComponentName)
        {
            return SkeletalMeshComponent;
        }
    }

    return BestCandidate;
}
