#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SquadAIPackAutoCharacterComponent.generated.h"

class UMeshComponent;
class USkeletalMeshComponent;
class ALeaderCharacter;
class AAllySoldier;

UCLASS(ClassGroup=(SquadAI), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class SQUADAI_API USquadAIPackAutoCharacterComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USquadAIPackAutoCharacterComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="SquadAI|Pack Setup")
    bool RunAutoSetup();

    UFUNCTION(BlueprintCallable, Category="SquadAI|Pack Setup")
    UMeshComponent* ResolveWeaponMeshComponent() const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup")
    bool bAutoSetupOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup")
    bool bTreatAsLeader = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup")
    bool bEnableLeftHandIK = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup")
    FName PreferredWeaponMeshComponentName = TEXT("WeaponMesh");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup")
    FName PreferredWeaponMeshTag = TEXT("TPWCS_WeaponMesh");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup")
    bool bRetryIfSetupFails = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup", meta=(ClampMin="0.01", UIMin="0.01"))
    float RetryDelay = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup", meta=(ClampMin="0", UIMin="0"))
    int32 MaxRetryCount = 10;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="SquadAI|Pack Setup")
    bool bSetupSucceeded = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="SquadAI|Pack Setup")
    int32 AttemptCount = 0;

private:
    void TryAutoSetup();
    void ScheduleRetry();
    UMeshComponent* FindWeaponMeshByTag(AActor* OwnerActor) const;
    UMeshComponent* FindWeaponMeshByName(AActor* OwnerActor) const;
    UMeshComponent* FindFallbackWeaponMesh(AActor* OwnerActor) const;

    FTimerHandle RetryTimerHandle;
};
