#include "Components/SquadAIPackValidationComponent.h"

#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SquadAIPackAutoCharacterComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"

USquadAIPackValidationComponent::USquadAIPackValidationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USquadAIPackValidationComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bValidateOnBeginPlay)
    {
        ValidateCurrentOwner(true);
    }
}

FSquadAIPackValidationResult USquadAIPackValidationComponent::ValidateCurrentOwner(bool bPrintResults) const
{
    FSquadAIPackValidationResult Result;

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        Result.MissingItems.Add(TEXT("Owner actor is null"));
        if (bPrintResults)
        {
            PrintValidationMessage(TEXT("Validation failed: owner actor is null."), true);
        }
        return Result;
    }

    Result.bHasWeaponMesh = ResolveWeaponMesh(OwnerActor) != nullptr;
    if (!Result.bHasWeaponMesh)
    {
        Result.MissingItems.Add(TEXT("Missing WeaponMesh component or TPWCS_WeaponMesh-tagged mesh"));
    }

    TArray<UActorComponent*> ActorComponents;
    OwnerActor->GetComponents(ActorComponents);

    UPrimitiveComponent* RagdollCapsule = nullptr;
    UWidgetComponent* HealthbarWidget = nullptr;

    for (UActorComponent* Component : ActorComponents)
    {
        if (!Component)
        {
            continue;
        }

        const FString ClassName = Component->GetClass()->GetName();
        if (ClassName.Contains(TEXT("BPC_WeaponCharacterAI")))
        {
            Result.bHasWeaponCharacterAIComponent = true;
        }
        if (ClassName.Contains(TEXT("BPC_HealthAI")) || ClassName.Contains(TEXT("HealthComponent")))
        {
            Result.bHasHealthComponent = true;
        }

        if (!RagdollCapsule)
        {
            RagdollCapsule = Cast<UPrimitiveComponent>(Component);
            if (RagdollCapsule && !RagdollCapsule->ComponentHasTag(ExpectedRagdollCapsuleTag))
            {
                RagdollCapsule = nullptr;
            }
        }

        if (!HealthbarWidget)
        {
            UWidgetComponent* WidgetComp = Cast<UWidgetComponent>(Component);
            if (WidgetComp && WidgetComp->ComponentHasTag(ExpectedHealthbarTag))
            {
                HealthbarWidget = WidgetComp;
            }
        }
    }

    Result.bHasRagdollCapsuleTag = (RagdollCapsule != nullptr);
    Result.bHasHealthbarTag = (HealthbarWidget != nullptr);

    if (!Result.bHasWeaponCharacterAIComponent)
    {
        Result.MissingItems.Add(TEXT("Missing BPC_WeaponCharacterAI component"));
    }
    if (!Result.bHasHealthComponent)
    {
        Result.MissingItems.Add(TEXT("Missing health component (BPC_HealthAI or SquadAI HealthComponent)"));
    }
    if (!Result.bHasRagdollCapsuleTag)
    {
        Result.MissingItems.Add(TEXT("Missing ragdoll capsule tag TPWCS_RagdollCapsule"));
    }
    if (!Result.bHasHealthbarTag)
    {
        Result.MissingItems.Add(TEXT("Missing healthbar widget tag TPWCS_AIHealthbar"));
    }

    if (APawn* PawnOwner = Cast<APawn>(OwnerActor))
    {
        Result.bHasAIControllerClass = PawnOwner->AIControllerClass != nullptr;
    }
    else
    {
        Result.bHasAIControllerClass = true;
    }

    if (!Result.bHasAIControllerClass)
    {
        Result.MissingItems.Add(TEXT("Missing AI Controller Class on pawn defaults"));
    }

    if (UMeshComponent* WeaponMesh = ResolveWeaponMesh(OwnerActor))
    {
        Result.bHasWeaponMeshTag = WeaponMesh->ComponentHasTag(ExpectedWeaponMeshTag);
    }

    if (!Result.bHasWeaponMeshTag)
    {
        Result.MissingItems.Add(TEXT("Weapon mesh is missing TPWCS_WeaponMesh tag"));
    }

    Result.bOverallPass =
        Result.bHasWeaponMesh &&
        Result.bHasWeaponCharacterAIComponent &&
        Result.bHasHealthComponent &&
        Result.bHasAIControllerClass &&
        Result.bHasWeaponMeshTag &&
        Result.bHasRagdollCapsuleTag &&
        Result.bHasHealthbarTag;

    if (bPrintResults)
    {
        if (Result.bOverallPass)
        {
            if (bPrintSuccesses)
            {
                PrintValidationMessage(FString::Printf(TEXT("[%s] Pack validation passed."), *OwnerActor->GetName()), false);
            }
        }
        else
        {
            PrintValidationMessage(FString::Printf(TEXT("[%s] Pack validation failed:"), *OwnerActor->GetName()), true);
            for (const FString& MissingItem : Result.MissingItems)
            {
                PrintValidationMessage(FString::Printf(TEXT(" - %s"), *MissingItem), true);
            }
        }
    }

    return Result;
}

UMeshComponent* USquadAIPackValidationComponent::ResolveWeaponMesh(AActor* OwnerActor) const
{
    if (!OwnerActor)
    {
        return nullptr;
    }

    if (const USquadAIPackAutoCharacterComponent* AutoComp = OwnerActor->FindComponentByClass<USquadAIPackAutoCharacterComponent>())
    {
        if (UMeshComponent* Resolved = AutoComp->ResolveWeaponMeshComponent())
        {
            return Resolved;
        }
    }

    TArray<UMeshComponent*> MeshComponents;
    OwnerActor->GetComponents<UMeshComponent>(MeshComponents);
    for (UMeshComponent* MeshComponent : MeshComponents)
    {
        if (MeshComponent && (MeshComponent->ComponentHasTag(ExpectedWeaponMeshTag) || MeshComponent->GetFName() == TEXT("WeaponMesh")))
        {
            return MeshComponent;
        }
    }

    return nullptr;
}

void USquadAIPackValidationComponent::PrintValidationMessage(const FString& Message, bool bIsError) const
{
    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            INDEX_NONE,
            bIsError ? 8.0f : 4.0f,
            bIsError ? FColor::Red : FColor::Green,
            Message);
    }
}
