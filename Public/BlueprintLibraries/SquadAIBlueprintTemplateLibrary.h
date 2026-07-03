#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SquadAIBlueprintTemplateLibrary.generated.h"

class AActor;

UCLASS()
class SQUADAI_API USquadAIBlueprintTemplateLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Template")
    static bool AddMissingPackValidationComponent(AActor* Actor);

    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Template")
    static bool AddMissingPackAutoSetupComponent(AActor* Actor);

    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Template")
    static bool AddMissingLeaderTeamAutoSetupComponent(AActor* Actor);

    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Template")
    static bool AutoPrepareActorTemplate(AActor* Actor, bool bTreatAsLeader = false);
};
