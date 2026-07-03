#include "Actors/SquadAIAutoSetupManager.h"

#include "BlueprintLibraries/SquadAIAutoSetupEditorLibrary.h"
#include "BlueprintLibraries/SquadAIBlueprintAssetFactoryLibrary.h"
#include "Characters/AllySoldier.h"
#include "Characters/EnemySoldier.h"
#include "Characters/LeaderCharacter.h"
#include "Engine/Blueprint.h"
#include "EngineUtils.h"
#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

namespace
{
    static bool GSquadAIVerboseDebugLogsEnabled = false;
}

ASquadAIAutoSetupManager::ASquadAIAutoSetupManager()
{
    PrimaryActorTick.bCanEverTick = false;
    bIsEditorOnlyActor = false;
}

bool ASquadAIAutoSetupManager::AreVerboseDebugLogsEnabled()
{
    return GSquadAIVerboseDebugLogsEnabled;
}

void ASquadAIAutoSetupManager::BeginPlay()
{
    Super::BeginPlay();
    GSquadAIVerboseDebugLogsEnabled = bEnableVerboseDebugLogs;

    if (!bRunOnBeginPlay || bRunInEditorOnly || bSetupAlreadyCompleted)
    {
        return;
    }

    if (RunEditorAutoSetup())
    {
        bSetupAlreadyCompleted = true;
        if (bValidateAfterSetup)
        {
            RunEditorValidation();
        }
        DisableManagerIfNeeded();
    }
}

void ASquadAIAutoSetupManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
#if WITH_EDITOR
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.GetPropertyName();
    GSquadAIVerboseDebugLogsEnabled = bEnableVerboseDebugLogs;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(ASquadAIAutoSetupManager, bExecuteCreateLeaderBlueprint) && bExecuteCreateLeaderBlueprint)
    {
        CreateLeaderBlueprintAsset();
        bExecuteCreateLeaderBlueprint = false;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(ASquadAIAutoSetupManager, bExecuteCreateAllyBlueprint) && bExecuteCreateAllyBlueprint)
    {
        CreateAllyBlueprintAsset();
        bExecuteCreateAllyBlueprint = false;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(ASquadAIAutoSetupManager, bExecuteCreateEnemyBlueprint) && bExecuteCreateEnemyBlueprint)
    {
        CreateEnemyBlueprintAsset();
        bExecuteCreateEnemyBlueprint = false;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(ASquadAIAutoSetupManager, bExecuteRunEditorAutoSetup) && bExecuteRunEditorAutoSetup)
    {
        RunEditorAutoSetup();
        bExecuteRunEditorAutoSetup = false;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(ASquadAIAutoSetupManager, bExecuteRunEditorValidation) && bExecuteRunEditorValidation)
    {
        RunEditorValidation();
        bExecuteRunEditorValidation = false;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(ASquadAIAutoSetupManager, bExecuteBuildSetupSummary) && bExecuteBuildSetupSummary)
    {
        LastSetupSummary = BuildSetupSummary();
        bExecuteBuildSetupSummary = false;
    }
#else
    Super::PostEditChangeProperty(PropertyChangedEvent);
#endif
}

bool ASquadAIAutoSetupManager::RunEditorAutoSetup()
{
    if (bSetupAlreadyCompleted)
    {
        return true;
    }

    ALeaderCharacter* Leader = ResolveLeader();
    if (!Leader)
    {
        return false;
    }

    const TArray<AActor*> Allies = ResolveAllies();
    const bool bSuccess = USquadAIAutoSetupEditorLibrary::AutoSetupLeaderTeamInEditor(Leader, Allies);
    if (bSuccess)
    {
        bSetupAlreadyCompleted = true;
        DisableManagerIfNeeded();
    }
    return bSuccess;
}

bool ASquadAIAutoSetupManager::RunEditorValidation()
{
    const FString Prefix = TEXT("[SquadAIAutoSetupManager]");
    bool bAllPassed = true;
    FString ValidationSummary = Prefix + TEXT(" Validation Results\n");

    if (ALeaderCharacter* Leader = ResolveLeader())
    {
        const bool bLeaderPassed = USquadAIAutoSetupEditorLibrary::ValidateActorForPackSetup(Leader);
        bAllPassed &= bLeaderPassed;
        ValidationSummary += FString::Printf(TEXT("Leader %s: %s\n"), *Leader->GetName(), bLeaderPassed ? TEXT("PASS") : TEXT("FAIL"));
    }
    else
    {
        bAllPassed = false;
        ValidationSummary += TEXT("Leader: MISSING\n");
    }

    const TArray<AActor*> Allies = ResolveAllies();
    for (AActor* Ally : Allies)
    {
        const bool bAllyPassed = USquadAIAutoSetupEditorLibrary::ValidateActorForPackSetup(Ally);
        bAllPassed &= bAllyPassed;
        ValidationSummary += FString::Printf(TEXT("Ally %s: %s\n"), *GetNameSafe(Ally), bAllyPassed ? TEXT("PASS") : TEXT("FAIL"));
    }

    const TArray<AActor*> Enemies = ResolveEnemies();
    for (AActor* Enemy : Enemies)
    {
        const bool bEnemyPassed = USquadAIAutoSetupEditorLibrary::ValidateActorForPackSetup(Enemy);
        bAllPassed &= bEnemyPassed;
        ValidationSummary += FString::Printf(TEXT("Enemy %s: %s\n"), *GetNameSafe(Enemy), bEnemyPassed ? TEXT("PASS") : TEXT("FAIL"));
    }

    ValidationSummary += FString::Printf(TEXT("Overall Validation: %s\n"), bAllPassed ? TEXT("PASS") : TEXT("FAIL"));
    LastSetupSummary = ValidationSummary + BuildSetupSummary();

    if (bAllPassed)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Validation passed."), *Prefix);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s Validation failed."), *Prefix);
    }

    return bAllPassed;
}

UBlueprint* ASquadAIAutoSetupManager::CreateLeaderBlueprintAsset()
{
    if (bCreateBlueprintsFromPackTemplates)
    {
        if (UBlueprint* Duplicated = USquadAIBlueprintAssetFactoryLibrary::DuplicatePackTemplateAsLeaderBlueprintAsset(LeaderPackTemplateAssetName, AssetPackagePath, LeaderAssetName))
        {
            return Duplicated;
        }
    }

    return USquadAIBlueprintAssetFactoryLibrary::CreateLeaderPackBlueprintAsset(AssetPackagePath, LeaderAssetName);
}

UBlueprint* ASquadAIAutoSetupManager::CreateAllyBlueprintAsset()
{
    if (bCreateBlueprintsFromPackTemplates)
    {
        if (UBlueprint* Duplicated = USquadAIBlueprintAssetFactoryLibrary::DuplicatePackTemplateAsAllyBlueprintAsset(AllyPackTemplateAssetName, AssetPackagePath, AllyAssetName))
        {
            return Duplicated;
        }
    }

    return USquadAIBlueprintAssetFactoryLibrary::CreateAllyPackBlueprintAsset(AssetPackagePath, AllyAssetName);
}

UBlueprint* ASquadAIAutoSetupManager::CreateEnemyBlueprintAsset()
{
    if (bCreateBlueprintsFromPackTemplates)
    {
        if (UBlueprint* Duplicated = USquadAIBlueprintAssetFactoryLibrary::DuplicatePackTemplateAsEnemyBlueprintAsset(EnemyPackTemplateAssetName, AssetPackagePath, EnemyAssetName))
        {
            return Duplicated;
        }
    }

    return USquadAIBlueprintAssetFactoryLibrary::CreateEnemyPackBlueprintAsset(AssetPackagePath, EnemyAssetName);
}

FString ASquadAIAutoSetupManager::BuildSetupSummary()
{
    FString Summary;
    Summary += FString::Printf(TEXT("Manager: %s\n"), *GetName());
    Summary += FString::Printf(TEXT("Setup Completed: %s\n"), bSetupAlreadyCompleted ? TEXT("Yes") : TEXT("No"));

    if (ALeaderCharacter* Leader = ResolveLeader())
    {
        Summary += FString::Printf(TEXT("Leader: %s\n"), *Leader->GetName());
    }
    else
    {
        Summary += TEXT("Leader: MISSING\n");
    }

    const TArray<AActor*> Allies = ResolveAllies();
    Summary += FString::Printf(TEXT("Allies Found: %d\n"), Allies.Num());
    for (AActor* Ally : Allies)
    {
        if (Ally)
        {
            Summary += FString::Printf(TEXT(" - %s\n"), *Ally->GetName());
        }
    }

    const TArray<AActor*> Enemies = ResolveEnemies();
    Summary += FString::Printf(TEXT("Enemies Found: %d\n"), Enemies.Num());
    for (AActor* Enemy : Enemies)
    {
        if (Enemy)
        {
            Summary += FString::Printf(TEXT(" - %s\n"), *Enemy->GetName());
        }
    }

    LastSetupSummary = Summary;
    return Summary;
}

ALeaderCharacter* ASquadAIAutoSetupManager::ResolveLeader() const
{
    if (IsValid(ExplicitLeader))
    {
        return ExplicitLeader;
    }

    if (!bAutoFindLeaderIfMissing || !GetWorld())
    {
        return nullptr;
    }

    for (TActorIterator<ALeaderCharacter> It(GetWorld()); It; ++It)
    {
        if (!RequiredActorTag.IsNone() && !It->ActorHasTag(RequiredActorTag))
        {
            continue;
        }

        return *It;
    }

    return nullptr;
}

TArray<AActor*> ASquadAIAutoSetupManager::ResolveAllies() const
{
    TArray<AActor*> Result;

    for (AAllySoldier* Ally : ExplicitAllies)
    {
        if (IsValid(Ally))
        {
            Result.Add(Ally);
        }
    }

    if (Result.Num() > 0 || !bAutoFindAlliesIfArrayEmpty || !GetWorld())
    {
        return Result;
    }

    for (TActorIterator<AAllySoldier> It(GetWorld()); It; ++It)
    {
        if (!RequiredActorTag.IsNone() && !It->ActorHasTag(RequiredActorTag))
        {
            continue;
        }

        Result.Add(*It);
    }

    return Result;
}

TArray<AActor*> ASquadAIAutoSetupManager::ResolveEnemies() const
{
    TArray<AActor*> Result;

    for (AEnemySoldier* Enemy : ExplicitEnemies)
    {
        if (IsValid(Enemy))
        {
            Result.Add(Enemy);
        }
    }

    if (Result.Num() > 0 || !bAutoFindEnemiesIfArrayEmpty || !GetWorld())
    {
        return Result;
    }

    for (TActorIterator<AEnemySoldier> It(GetWorld()); It; ++It)
    {
        if (!RequiredActorTag.IsNone() && !It->ActorHasTag(RequiredActorTag))
        {
            continue;
        }

        Result.Add(*It);
    }

    return Result;
}

void ASquadAIAutoSetupManager::DisableManagerIfNeeded()
{
    if (!bDisableAfterSuccessfulSetup)
    {
        return;
    }

    SetActorTickEnabled(false);
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}
