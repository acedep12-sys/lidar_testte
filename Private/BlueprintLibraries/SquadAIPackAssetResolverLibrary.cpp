#include "BlueprintLibraries/SquadAIPackAssetResolverLibrary.h"

#include "Engine/Blueprint.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#endif

UClass* USquadAIPackAssetResolverLibrary::ResolvePackComponentClassByName(const FString& AssetName)
{
#if WITH_EDITOR
    UBlueprint* Blueprint = ResolvePackBlueprintByName(AssetName);
    return Blueprint ? Blueprint->GeneratedClass : nullptr;
#else
    return nullptr;
#endif
}

UClass* USquadAIPackAssetResolverLibrary::ResolveBlueprintClassByName(const FString& AssetName)
{
#if WITH_EDITOR
    UBlueprint* Blueprint = ResolvePackBlueprintByName(AssetName);
    return Blueprint ? Blueprint->GeneratedClass : nullptr;
#else
    return nullptr;
#endif
}

UBlueprint* USquadAIPackAssetResolverLibrary::ResolvePackBlueprintByName(const FString& AssetName)
{
#if WITH_EDITOR
    if (AssetName.IsEmpty())
    {
        return nullptr;
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    Filter.PackagePaths.Add(TEXT("/Game"));

    TArray<FAssetData> Assets;
    AssetRegistryModule.Get().GetAssets(Filter, Assets);

    for (const FAssetData& AssetData : Assets)
    {
        if (AssetData.AssetName.ToString().Equals(AssetName, ESearchCase::CaseSensitive))
        {
            return Cast<UBlueprint>(AssetData.GetAsset());
        }
    }
#endif

    return nullptr;
}
