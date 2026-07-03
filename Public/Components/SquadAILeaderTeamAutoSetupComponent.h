#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SquadAILeaderTeamAutoSetupComponent.generated.h"

class AActor;

UCLASS(ClassGroup=(SquadAI), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class SQUADAI_API USquadAILeaderTeamAutoSetupComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USquadAILeaderTeamAutoSetupComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="SquadAI|Pack Setup")
    bool RunLeaderTeamSetup();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup")
    bool bAutoSetupOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup")
    TArray<AActor*> AllyActors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup", meta=(ClampMin="0", UIMin="0"))
    int32 StartingFormationSlot = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup")
    bool bAutoBuildWaypointsFromQuest = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup")
    bool bAutoBeginQuest = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup")
    bool bSetupAlliesForPack = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup")
    bool bRetryIfSetupFails = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup", meta=(ClampMin="0.01", UIMin="0.01"))
    float InitialSetupDelay = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup", meta=(ClampMin="0.01", UIMin="0.01"))
    float RetryDelay = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SquadAI|Pack Setup", meta=(ClampMin="0", UIMin="0"))
    int32 MaxRetryCount = 8;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="SquadAI|Pack Setup")
    bool bSetupSucceeded = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="SquadAI|Pack Setup")
    int32 AttemptCount = 0;

private:
    void BeginDelayedSetup();
    void TryLeaderTeamSetup();
    void ScheduleRetry();
    TArray<AActor*> BuildValidAllies() const;

    FTimerHandle InitialSetupTimerHandle;
    FTimerHandle RetryTimerHandle;
};
