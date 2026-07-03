#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SquadAIBlueprintAssetFactoryLibrary.generated.h"

class UBlueprint;

UCLASS()
class SQUADAI_API USquadAIBlueprintAssetFactoryLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Asset Factory")
    static UBlueprint* CreateLeaderPackBlueprintAsset(const FString& PackagePath, const FString& AssetName);

    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Asset Factory")
    static UBlueprint* CreateAllyPackBlueprintAsset(const FString& PackagePath, const FString& AssetName);

    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Asset Factory")
    static UBlueprint* CreateEnemyPackBlueprintAsset(const FString& PackagePath, const FString& AssetName);

    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Asset Factory")
    static UBlueprint* DuplicatePackTemplateAsLeaderBlueprintAsset(const FString& TemplateBlueprintAssetName, const FString& PackagePath, const FString& AssetName);

    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Asset Factory")
    static UBlueprint* DuplicatePackTemplateAsAllyBlueprintAsset(const FString& TemplateBlueprintAssetName, const FString& PackagePath, const FString& AssetName);

    UFUNCTION(BlueprintCallable, CallInEditor, Category="SquadAI|Asset Factory")
    static UBlueprint* DuplicatePackTemplateAsEnemyBlueprintAsset(const FString& TemplateBlueprintAssetName, const FString& PackagePath, const FString& AssetName);
};
