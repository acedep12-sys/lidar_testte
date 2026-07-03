#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SquadAIPackValidationComponent.generated.h"

class UMeshComponent;

USTRUCT(BlueprintType)
struct FSquadAIPackValidationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SquadAI|Validation")
    bool bHasWeaponMesh = false;

    UPROPERTY(BlueprintReadOnly, Category="SquadAI|Validation")
    bool bHasWeaponCharacterAIComponent = false;

    UPROPERTY(BlueprintReadOnly, Category="SquadAI|Validation")
    bool bHasHealthComponent = false;

    UPROPERTY(BlueprintReadOnly, Category="SquadAI|Validation")
    bool bHasAIControllerClass = false;

    UPROPERTY(BlueprintReadOnly, Category="SquadAI|Validation")
    bool bHasWeaponMeshTag = false;

    UPROPERTY(BlueprintReadOnly, Category="SquadAI|Validation")
    bool bHasRagdollCapsuleTag = false;

    UPROPERTY(BlueprintReadOnly, Category="SquadAI|Validation")
    bool bHasHealthbarTag = false;

    UPROPERTY(BlueprintReadOnly, Category="SquadAI|Validation")
    bool bOverallPass = false;

    UPROPERTY(BlueprintReadOnly, Category="SquadAI|Validation")
    TArray<FString> MissingItems;
};

UCLASS(ClassGroup=(SquadAI), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class SQUADAI_API USquadAIPackValidationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USquadAIPackValidationComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="SquadAI|Validation")
    FSquadAIPackValidationResult ValidateCurrentOwner(bool bPrintResults = true) const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Validation")
    bool bValidateOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Validation")
    bool bPrintSuccesses = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Validation")
    FName ExpectedWeaponMeshTag = TEXT("TPWCS_WeaponMesh");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Validation")
    FName ExpectedRagdollCapsuleTag = TEXT("TPWCS_RagdollCapsule");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Validation")
    FName ExpectedHealthbarTag = TEXT("TPWCS_AIHealthbar");

private:
    UMeshComponent* ResolveWeaponMesh(AActor* OwnerActor) const;
    void PrintValidationMessage(const FString& Message, bool bIsError) const;
};
