#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SquadAIPackAssetResolverLibrary.generated.h"

class UClass;
class UBlueprint;

UCLASS()
class SQUADAI_API USquadAIPackAssetResolverLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="SquadAI|Asset Resolver")
    static UClass* ResolvePackComponentClassByName(const FString& AssetName);

    UFUNCTION(BlueprintCallable, Category="SquadAI|Asset Resolver")
    static UBlueprint* ResolvePackBlueprintByName(const FString& AssetName);

    UFUNCTION(BlueprintCallable, Category="SquadAI|Asset Resolver")
    static UClass* ResolveBlueprintClassByName(const FString& AssetName);
};
