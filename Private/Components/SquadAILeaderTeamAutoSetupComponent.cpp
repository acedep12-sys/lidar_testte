#include "Components/SquadAILeaderTeamAutoSetupComponent.h"

#include "BlueprintLibraries/SquadAIPackBridgeLibrary.h"
#include "Components/SquadAIPackAutoCharacterComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

USquadAILeaderTeamAutoSetupComponent::USquadAILeaderTeamAutoSetupComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USquadAILeaderTeamAutoSetupComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoSetupOnBeginPlay)
    {
        BeginDelayedSetup();
    }
}

bool USquadAILeaderTeamAutoSetupComponent::RunLeaderTeamSetup()
{
    ++AttemptCount;

    AActor* LeaderActor = GetOwner();
    if (!LeaderActor)
    {
        return false;
    }

    TArray<AActor*> ValidAllies = BuildValidAllies();
    bool bSuccess = true;

    if (USquadAIPackAutoCharacterComponent* LeaderPackSetup = LeaderActor->FindComponentByClass<USquadAIPackAutoCharacterComponent>())
    {
        bSuccess &= LeaderPackSetup->RunAutoSetup();
    }

    if (bSetupAlliesForPack)
    {
        for (AActor* AllyActor : ValidAllies)
        {
            if (!IsValid(AllyActor))
            {
                bSuccess = false;
                continue;
            }

            if (USquadAIPackAutoCharacterComponent* AllyPackSetup = AllyActor->FindComponentByClass<USquadAIPackAutoCharacterComponent>())
            {
                bSuccess &= AllyPackSetup->RunAutoSetup();
            }
        }
    }

    const bool bConfigured = USquadAIPackBridgeLibrary::AutoSetupExistingLeaderTeamFromQuest(
        LeaderActor,
        ValidAllies,
        StartingFormationSlot,
        bAutoBuildWaypointsFromQuest,
        bAutoBeginQuest);

    bSuccess &= bConfigured;
    bSetupSucceeded = bSuccess;

    if (bSetupSucceeded)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(InitialSetupTimerHandle);
            World->GetTimerManager().ClearTimer(RetryTimerHandle);
        }
    }

    return bSetupSucceeded;
}

void USquadAILeaderTeamAutoSetupComponent::BeginDelayedSetup()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    World->GetTimerManager().SetTimer(
        InitialSetupTimerHandle,
        this,
        &USquadAILeaderTeamAutoSetupComponent::TryLeaderTeamSetup,
        InitialSetupDelay,
        false);
}

void USquadAILeaderTeamAutoSetupComponent::TryLeaderTeamSetup()
{
    if (bSetupSucceeded)
    {
        return;
    }

    if (!RunLeaderTeamSetup() && bRetryIfSetupFails)
    {
        ScheduleRetry();
    }
}

void USquadAILeaderTeamAutoSetupComponent::ScheduleRetry()
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
        &USquadAILeaderTeamAutoSetupComponent::TryLeaderTeamSetup,
        RetryDelay,
        false);
}

TArray<AActor*> USquadAILeaderTeamAutoSetupComponent::BuildValidAllies() const
{
    TArray<AActor*> Result;
    Result.Reserve(AllyActors.Num());

    for (AActor* AllyActor : AllyActors)
    {
        if (IsValid(AllyActor) && AllyActor != GetOwner())
        {
            Result.Add(AllyActor);
        }
    }

    return Result;
}
