#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SquadAIAutoSetupEditorLibrary.generated.h"

class AActor;
class UBlueprint;

UCLASS()
class SQUADAI_API USquadAIAutoSetupEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Editor Setup")
    static bool ValidateActorForPackSetup(AActor* Actor);

    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Editor Setup")
    static bool AutoSetupActorForPackInEditor(AActor* Actor, bool bTreatAsLeader = false);

    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Editor Setup")
    static bool AutoSetupLeaderTeamInEditor(AActor* LeaderActor, const TArray<AActor*>& AllyActors);
};
