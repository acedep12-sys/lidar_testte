#include "BlueprintLibraries/SquadAIBlueprintTemplateLibrary.h"

#include "Characters/LeaderCharacter.h"
#include "Components/SquadAILeaderTeamAutoSetupComponent.h"
#include "Components/SquadAIPackAutoCharacterComponent.h"
#include "Components/SquadAIPackValidationComponent.h"
#include "GameFramework/Actor.h"

bool USquadAIBlueprintTemplateLibrary::AddMissingPackValidationComponent(AActor* Actor)
{
    if (!Actor)
    {
        return false;
    }

    if (Actor->FindComponentByClass<USquadAIPackValidationComponent>())
    {
        return true;
    }

    USquadAIPackValidationComponent* Component = NewObject<USquadAIPackValidationComponent>(Actor, TEXT("SquadAIPackValidationComponent_Auto"));
    if (!Component)
    {
        return false;
    }

    Component->RegisterComponent();
    return true;
}

bool USquadAIBlueprintTemplateLibrary::AddMissingPackAutoSetupComponent(AActor* Actor)
{
    if (!Actor)
    {
        return false;
    }

    if (Actor->FindComponentByClass<USquadAIPackAutoCharacterComponent>())
    {
        return true;
    }

    USquadAIPackAutoCharacterComponent* Component = NewObject<USquadAIPackAutoCharacterComponent>(Actor, TEXT("SquadAIPackAutoCharacterComponent_Auto"));
    if (!Component)
    {
        return false;
    }

    Component->RegisterComponent();
    return true;
}

bool USquadAIBlueprintTemplateLibrary::AddMissingLeaderTeamAutoSetupComponent(AActor* Actor)
{
    if (!Actor)
    {
        return false;
    }

    if (Actor->FindComponentByClass<USquadAILeaderTeamAutoSetupComponent>())
    {
        return true;
    }

    USquadAILeaderTeamAutoSetupComponent* Component = NewObject<USquadAILeaderTeamAutoSetupComponent>(Actor, TEXT("SquadAILeaderTeamAutoSetupComponent_Auto"));
    if (!Component)
    {
        return false;
    }

    Component->RegisterComponent();
    return true;
}

bool USquadAIBlueprintTemplateLibrary::AutoPrepareActorTemplate(AActor* Actor, bool bTreatAsLeader)
{
    if (!Actor)
    {
        return false;
    }

    bool bSuccess = true;
    bSuccess &= AddMissingPackAutoSetupComponent(Actor);
    bSuccess &= AddMissingPackValidationComponent(Actor);

    if (bTreatAsLeader || Actor->IsA<ALeaderCharacter>())
    {
        bSuccess &= AddMissingLeaderTeamAutoSetupComponent(Actor);
    }

    return bSuccess;
}
