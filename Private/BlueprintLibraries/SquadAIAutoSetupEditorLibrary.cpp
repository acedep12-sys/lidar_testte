#include "BlueprintLibraries/SquadAIAutoSetupEditorLibrary.h"

#include "BlueprintLibraries/SquadAIPackBridgeLibrary.h"
#include "Components/SquadAILeaderTeamAutoSetupComponent.h"
#include "Components/SquadAIPackAutoCharacterComponent.h"
#include "Components/SquadAIPackValidationComponent.h"
#include "GameFramework/Actor.h"

bool USquadAIAutoSetupEditorLibrary::ValidateActorForPackSetup(AActor* Actor)
{
    if (!Actor)
    {
        return false;
    }

    USquadAIPackValidationComponent* Validator = Actor->FindComponentByClass<USquadAIPackValidationComponent>();
    if (!Validator)
    {
        Validator = NewObject<USquadAIPackValidationComponent>(Actor, TEXT("SquadAIPackValidationComponent_Auto"));
        if (!Validator)
        {
            return false;
        }
        Validator->RegisterComponent();
    }

    return Validator->ValidateCurrentOwner(true).bOverallPass;
}

bool USquadAIAutoSetupEditorLibrary::AutoSetupActorForPackInEditor(AActor* Actor, bool bTreatAsLeader)
{
    if (!Actor)
    {
        return false;
    }

    USquadAIPackAutoCharacterComponent* AutoComp = Actor->FindComponentByClass<USquadAIPackAutoCharacterComponent>();
    if (!AutoComp)
    {
        AutoComp = NewObject<USquadAIPackAutoCharacterComponent>(Actor, TEXT("SquadAIPackAutoCharacterComponent_Auto"));
        if (!AutoComp)
        {
            return false;
        }
        AutoComp->RegisterComponent();
    }

    if (bTreatAsLeader)
    {
        // Property is editor-visible; this call path intentionally relies on leader class too.
    }

    return AutoComp->RunAutoSetup();
}

bool USquadAIAutoSetupEditorLibrary::AutoSetupLeaderTeamInEditor(AActor* LeaderActor, const TArray<AActor*>& AllyActors)
{
    if (!LeaderActor)
    {
        return false;
    }

    const bool bLeaderSetup = AutoSetupActorForPackInEditor(LeaderActor, true);

    bool bAlliesSetup = true;
    for (AActor* AllyActor : AllyActors)
    {
        bAlliesSetup &= AutoSetupActorForPackInEditor(AllyActor, false);
    }

    const bool bConfigured = USquadAIPackBridgeLibrary::AutoConfigureExistingLeaderTeam(
        LeaderActor,
        AllyActors,
        0);

    const bool bWaypointsBuilt = USquadAIPackBridgeLibrary::AutoBuildLeaderWaypointsFromQuest(
        LeaderActor,
        true);

    return bLeaderSetup && bAlliesSetup && bConfigured && bWaypointsBuilt;
}
