#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SquadAIPackLoadoutBridgeComponent.generated.h"

UCLASS(ClassGroup=(SquadAI), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class SQUADAI_API USquadAIPackLoadoutBridgeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USquadAIPackLoadoutBridgeComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="SquadAI|Pack Loadout")
    bool TryApplyPackLoadout();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Loadout")
    bool bAutoApplyOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Loadout")
    TArray<FName> WeaponIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Loadout")
    bool bChooseRandomWeapon = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Loadout")
    int32 Ammo = 999;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Loadout")
    bool bEquip = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Loadout")
    bool bStopMeleeAfterGive = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Loadout", meta=(ClampMin="0.0", UIMin="0.0"))
    float BeginPlayDelay = 0.1f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="SquadAI|Pack Loadout")
    bool bLoadoutApplied = false;

private:
    void DelayedApply();
};
