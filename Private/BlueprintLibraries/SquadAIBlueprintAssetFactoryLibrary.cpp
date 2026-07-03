#include "BlueprintLibraries/SquadAIBlueprintAssetFactoryLibrary.h"
#include "BlueprintLibraries/SquadAIPackAssetResolverLibrary.h"

#include "Blueprint/UserWidget.h"
#include "Characters/AllySoldier.h"
#include "Characters/EnemySoldier.h"
#include "Characters/LeaderCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SquadAILeaderTeamAutoSetupComponent.h"
#include "Components/SquadAIPackAutoCharacterComponent.h"
#include "Components/SquadAIPackLoadoutBridgeComponent.h"
#include "Components/SquadAIPackValidationComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "GameFramework/Character.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/Blueprint.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#endif

namespace SquadAIBlueprintAssetFactoryInternal
{
#if WITH_EDITOR
    static void AddComponentNode(UBlueprint* Blueprint, UClass* ComponentClass, const FName NodeName, USCS_Node*& OutNode)
    {
        OutNode = nullptr;
        if (!Blueprint || !Blueprint->SimpleConstructionScript || !ComponentClass)
        {
            return;
        }

        for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (Node && Node->GetVariableName() == NodeName)
            {
                OutNode = Node;
                return;
            }
        }

        OutNode = Blueprint->SimpleConstructionScript->CreateNode(ComponentClass, NodeName);
        if (OutNode)
        {
            Blueprint->SimpleConstructionScript->AddNode(OutNode);
        }
    }

    static void AttachNodeToParent(UBlueprint* Blueprint, USCS_Node* ChildNode, const FName ParentVariableName)
    {
        if (!Blueprint || !Blueprint->SimpleConstructionScript || !ChildNode)
        {
            return;
        }

        for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (Node && Node->GetVariableName() == ParentVariableName)
            {
                ChildNode->SetParent(Node);
                return;
            }
        }
    }

    static void AddTagToComponentNode(USCS_Node* Node, const FName TagName)
    {
        if (!Node || TagName.IsNone())
        {
            return;
        }

        if (UActorComponent* Template = Cast<UActorComponent>(Node->ComponentTemplate))
        {
            Template->ComponentTags.AddUnique(TagName);
        }
    }

    static void ConfigureMeshNode(USCS_Node* MeshNode)
    {
        if (!MeshNode)
        {
            return;
        }

        if (USkeletalMeshComponent* MeshTemplate = Cast<USkeletalMeshComponent>(MeshNode->ComponentTemplate))
        {
            MeshTemplate->SetRelativeLocation(FVector(0.f, 0.000856f, -97.f));
            MeshTemplate->SetRelativeRotation(FRotator(0.f, 0.f, 270.f));
            MeshTemplate->SetRelativeScale3D(FVector(1.f));
        }
    }

    static void ConfigureRagdollCapsuleNode(USCS_Node* CapsuleNode)
    {
        if (!CapsuleNode)
        {
            return;
        }

        if (UCapsuleComponent* CapsuleTemplate = Cast<UCapsuleComponent>(CapsuleNode->ComponentTemplate))
        {
            CapsuleTemplate->SetCapsuleHalfHeight(100.f);
            CapsuleTemplate->SetCapsuleRadius(48.f);
            CapsuleTemplate->ComponentTags.AddUnique(TEXT("TPWCS_RagdollCapsule"));
        }
    }

    static void ConfigureWeaponMeshNode(USCS_Node* WeaponMeshNode)
    {
        if (!WeaponMeshNode)
        {
            return;
        }

        if (USkeletalMeshComponent* WeaponTemplate = Cast<USkeletalMeshComponent>(WeaponMeshNode->ComponentTemplate))
        {
            WeaponTemplate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            WeaponTemplate->ComponentTags.AddUnique(TEXT("TPWCS_WeaponMesh"));
        }
    }

    static void ConfigureHealthbarNode(USCS_Node* HealthbarNode)
    {
        if (!HealthbarNode)
        {
            return;
        }

        if (UWidgetComponent* WidgetTemplate = Cast<UWidgetComponent>(HealthbarNode->ComponentTemplate))
        {
            WidgetTemplate->ComponentTags.AddUnique(TEXT("TPWCS_AIHealthbar"));
        }
    }

    static UClass* ResolvePreferredComponentClass(const FString& ExactAssetName, UClass* FallbackClass)
    {
        if (UClass* ResolvedClass = USquadAIPackAssetResolverLibrary::ResolvePackComponentClassByName(ExactAssetName))
        {
            return ResolvedClass;
        }

        return FallbackClass;
    }

    static FString MakeCleanGamePackagePath(const FString& PackagePath)
    {
        FString CleanPackagePath = PackagePath;
        if (!CleanPackagePath.StartsWith(TEXT("/Game")))
        {
            CleanPackagePath = TEXT("/Game");
        }
        return CleanPackagePath;
    }

    static UBlueprint* FindExistingBlueprintAsset(const FString& PackagePath, const FString& AssetName)
    {
        const FString FullAssetPath = PackagePath / AssetName;
        if (UBlueprint* LoadedBlueprint = FindObject<UBlueprint>(nullptr, *FullAssetPath))
        {
            return LoadedBlueprint;
        }

        UPackage* ExistingPackage = FindPackage(nullptr, *FullAssetPath);
        if (ExistingPackage)
        {
            if (UBlueprint* ExistingBlueprint = FindObject<UBlueprint>(ExistingPackage, *AssetName))
            {
                return ExistingBlueprint;
            }
        }

        return nullptr;
    }

    static void SetBoolProperty(UObject* Object, const FName PropertyName, bool bValue)
    {
        if (Object)
        {
            if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName))
            {
                Prop->SetPropertyValue_InContainer(Object, bValue);
            }
        }
    }

    static void SetIntProperty(UObject* Object, const FName PropertyName, int32 Value)
    {
        if (Object)
        {
            if (FIntProperty* Prop = FindFProperty<FIntProperty>(Object->GetClass(), PropertyName))
            {
                Prop->SetPropertyValue_InContainer(Object, Value);
            }
        }
    }

    static void SetFloatProperty(UObject* Object, const FName PropertyName, float Value)
    {
        if (Object)
        {
            if (FFloatProperty* Prop = FindFProperty<FFloatProperty>(Object->GetClass(), PropertyName))
            {
                Prop->SetPropertyValue_InContainer(Object, Value);
            }
        }
    }

    static void SetNameArrayProperty(UObject* Object, const FName PropertyName, const TArray<FName>& Values)
    {
        if (!Object)
        {
            return;
        }

        FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName);
        if (!ArrayProp || !CastField<FNameProperty>(ArrayProp->Inner))
        {
            return;
        }

        void* ArrayPtr = ArrayProp->ContainerPtrToValuePtr<void>(Object);
        FScriptArrayHelper Helper(ArrayProp, ArrayPtr);
        Helper.EmptyAndAddValues(Values.Num());
        for (int32 Index = 0; Index < Values.Num(); ++Index)
        {
            FNameProperty* NameProp = CastFieldChecked<FNameProperty>(ArrayProp->Inner);
            NameProp->SetPropertyValue(Helper.GetRawPtr(Index), Values[Index]);
        }
    }

    static void ConfigureDefaultLoadoutBridgeNode(USCS_Node* LoadoutBridgeNode)
    {
        if (!LoadoutBridgeNode || !LoadoutBridgeNode->ComponentTemplate)
        {
            return;
        }

        UObject* Template = LoadoutBridgeNode->ComponentTemplate;
        SetBoolProperty(Template, TEXT("bAutoApplyOnBeginPlay"), true);
        SetBoolProperty(Template, TEXT("bChooseRandomWeapon"), false);
        SetBoolProperty(Template, TEXT("bEquip"), true);
        SetBoolProperty(Template, TEXT("bStopMeleeAfterGive"), true);
        SetIntProperty(Template, TEXT("Ammo"), 999);
        SetFloatProperty(Template, TEXT("BeginPlayDelay"), 0.2f);
        SetNameArrayProperty(Template, TEXT("WeaponIDs"), TArray<FName>{ FName(TEXT("ak")) });
    }

    static void AddSquadAIHelperNodes(UBlueprint* Blueprint, bool bAddLeaderTeamAutoSetup)
    {
        if (!Blueprint || !Blueprint->SimpleConstructionScript)
        {
            return;
        }

        USCS_Node* PackAutoNode = nullptr;
        USCS_Node* LoadoutBridgeNode = nullptr;
        USCS_Node* ValidationNode = nullptr;
        USCS_Node* LeaderTeamNode = nullptr;

        AddComponentNode(Blueprint, USquadAIPackAutoCharacterComponent::StaticClass(), TEXT("SquadAIPackAutoCharacterComponent"), PackAutoNode);
        AddComponentNode(Blueprint, USquadAIPackLoadoutBridgeComponent::StaticClass(), TEXT("SquadAIPackLoadoutBridgeComponent"), LoadoutBridgeNode);
        AddComponentNode(Blueprint, USquadAIPackValidationComponent::StaticClass(), TEXT("SquadAIPackValidationComponent"), ValidationNode);
        ConfigureDefaultLoadoutBridgeNode(LoadoutBridgeNode);

        if (bAddLeaderTeamAutoSetup)
        {
            AddComponentNode(Blueprint, USquadAILeaderTeamAutoSetupComponent::StaticClass(), TEXT("SquadAILeaderTeamAutoSetupComponent"), LeaderTeamNode);
        }
    }

    static void TagExistingPackWeaponComponentNodes(UBlueprint* Blueprint)
    {
        if (!Blueprint || !Blueprint->SimpleConstructionScript)
        {
            return;
        }

        for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (!Node || !Node->ComponentTemplate)
            {
                continue;
            }

            const FString NodeName = Node->GetVariableName().ToString();
            const FString ClassName = Node->ComponentTemplate->GetClass() ? Node->ComponentTemplate->GetClass()->GetName() : FString();
            if (NodeName.Contains(TEXT("BPC_Weapon")) || ClassName.Contains(TEXT("BPC_Weapon")) || ClassName.Contains(TEXT("WeaponCharacterAI")))
            {
                AddTagToComponentNode(Node, TEXT("TPWCS_BPCWeapon"));
            }
        }
    }

    static FString GetDefaultControllerBlueprintNameForParent(UClass* DesiredParentClass)
    {
        if (!DesiredParentClass)
        {
            return FString();
        }

        if (DesiredParentClass->IsChildOf(AEnemySoldier::StaticClass()))
        {
            return TEXT("BP_EnemyAIController");
        }
        if (DesiredParentClass->IsChildOf(AAllySoldier::StaticClass()))
        {
            return TEXT("BP_AllyAIController");
        }
        if (DesiredParentClass->IsChildOf(ALeaderCharacter::StaticClass()))
        {
            return TEXT("BP_LeaderAIController");
        }

        return FString();
    }

    static void ConfigureGeneratedCharacterDefaults(UBlueprint* Blueprint, UClass* DesiredParentClass)
    {
        if (!Blueprint || !Blueprint->GeneratedClass)
        {
            return;
        }

        ACharacter* CharacterCDO = Cast<ACharacter>(Blueprint->GeneratedClass->GetDefaultObject());
        if (!CharacterCDO)
        {
            return;
        }

        const FString ControllerBlueprintName = GetDefaultControllerBlueprintNameForParent(DesiredParentClass);
        if (!ControllerBlueprintName.IsEmpty())
        {
            if (UClass* ControllerClass = USquadAIPackAssetResolverLibrary::ResolveBlueprintClassByName(ControllerBlueprintName))
            {
                CharacterCDO->Modify();
                CharacterCDO->AIControllerClass = ControllerClass;
                CharacterCDO->AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
                UE_LOG(LogTemp, Warning, TEXT("[SquadAIBlueprintFactory] Set %s AIControllerClass to %s."), *GetNameSafe(Blueprint), *ControllerBlueprintName);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[SquadAIBlueprintFactory] Could not find controller BP '%s' for %s. Please set AI Controller manually."), *ControllerBlueprintName, *GetNameSafe(Blueprint));
            }
        }
    }

    static UBlueprint* DuplicatePackTemplateBlueprintAsset(UClass* DesiredParentClass, const FString& TemplateBlueprintAssetName, const FString& PackagePath, const FString& AssetName, bool bAddLeaderTeamAutoSetup)
    {
        if (!DesiredParentClass || TemplateBlueprintAssetName.IsEmpty() || PackagePath.IsEmpty() || AssetName.IsEmpty())
        {
            return nullptr;
        }

        const FString CleanPackagePath = MakeCleanGamePackagePath(PackagePath);
        if (UBlueprint* ExistingBlueprint = FindExistingBlueprintAsset(CleanPackagePath, AssetName))
        {
            return ExistingBlueprint;
        }

        UBlueprint* TemplateBlueprint = USquadAIPackAssetResolverLibrary::ResolvePackBlueprintByName(TemplateBlueprintAssetName);
        if (!TemplateBlueprint)
        {
            UE_LOG(LogTemp, Error, TEXT("[SquadAIBlueprintFactory] Could not find pack template blueprint named '%s'. Falling back to scratch creation."), *TemplateBlueprintAssetName);
            return nullptr;
        }

        FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

        // Critical TPWCS compatibility point:
        // BP_AICharacter is parented to the pack's BP_Character. If we reparent BP_AICharacter directly to
        // AEnemySoldier/AAllySoldier/ALeaderCharacter, we preserve its own SCS components but we lose the pack
        // BP_Character parent graph/functions/defaults. That can break weapon/animation behavior.
        // Safer route:
        //   1) duplicate the pack parent BP_Character as <AssetName>_PackBase,
        //   2) reparent that duplicate base to the desired SquadAI C++ class,
        //   3) duplicate BP_AICharacter and reparent it to the duplicated base's GeneratedClass.
        UClass* FinalParentClassForDuplicatedTemplate = DesiredParentClass;
        if (TemplateBlueprint->ParentClass)
        {
            if (UBlueprint* PackParentBlueprint = Cast<UBlueprint>(TemplateBlueprint->ParentClass->ClassGeneratedBy))
            {
                const FString ParentAssetName = AssetName + TEXT("_PackBase");
                UBlueprint* DuplicatedParentBlueprint = FindExistingBlueprintAsset(CleanPackagePath, ParentAssetName);
                if (!DuplicatedParentBlueprint)
                {
                    UObject* DuplicatedParentObject = AssetToolsModule.Get().DuplicateAsset(ParentAssetName, CleanPackagePath, PackParentBlueprint);
                    DuplicatedParentBlueprint = Cast<UBlueprint>(DuplicatedParentObject);
                }

                if (DuplicatedParentBlueprint)
                {
                    DuplicatedParentBlueprint->ParentClass = DesiredParentClass;
                    DuplicatedParentBlueprint->MarkPackageDirty();
                    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(DuplicatedParentBlueprint);
                    FKismetEditorUtilities::CompileBlueprint(DuplicatedParentBlueprint);

                    if (DuplicatedParentBlueprint->GeneratedClass)
                    {
                        FinalParentClassForDuplicatedTemplate = DuplicatedParentBlueprint->GeneratedClass;
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[SquadAIBlueprintFactory] Failed to duplicate pack parent blueprint '%s'. Falling back to direct reparent, which may lose pack parent BP logic."), *GetNameSafe(PackParentBlueprint));
                }
            }
        }

        UObject* DuplicatedObject = AssetToolsModule.Get().DuplicateAsset(AssetName, CleanPackagePath, TemplateBlueprint);
        UBlueprint* DuplicatedBlueprint = Cast<UBlueprint>(DuplicatedObject);
        if (!DuplicatedBlueprint)
        {
            UE_LOG(LogTemp, Error, TEXT("[SquadAIBlueprintFactory] Failed to duplicate template '%s' to '%s/%s'."), *TemplateBlueprintAssetName, *CleanPackagePath, *AssetName);
            return nullptr;
        }

        // Preserve the template's own SCS components/defaults and, when possible, preserve the pack BP parent logic
        // through the duplicated PackBase parent. The final generated class is still IsA(DesiredParentClass), so
        // existing SquadAI C++ casts continue to work.
        DuplicatedBlueprint->ParentClass = FinalParentClassForDuplicatedTemplate;
        AddSquadAIHelperNodes(DuplicatedBlueprint, bAddLeaderTeamAutoSetup);
        TagExistingPackWeaponComponentNodes(DuplicatedBlueprint);

        DuplicatedBlueprint->MarkPackageDirty();
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(DuplicatedBlueprint);
        FKismetEditorUtilities::CompileBlueprint(DuplicatedBlueprint);
        ConfigureGeneratedCharacterDefaults(DuplicatedBlueprint, DesiredParentClass);
        DuplicatedBlueprint->MarkPackageDirty();

        UE_LOG(LogTemp, Warning, TEXT("[SquadAIBlueprintFactory] Duplicated pack template '%s' -> '%s/%s' and reparented through '%s' to SquadAI base '%s'."),
            *TemplateBlueprintAssetName,
            *CleanPackagePath,
            *AssetName,
            *GetNameSafe(FinalParentClassForDuplicatedTemplate),
            *GetNameSafe(DesiredParentClass));

        return DuplicatedBlueprint;
    }

    static UBlueprint* CreatePackBlueprintAsset(UClass* ParentClass, const FString& PackagePath, const FString& AssetName, bool bAddLeaderTeamAutoSetup)
    {
        if (!ParentClass || PackagePath.IsEmpty() || AssetName.IsEmpty())
        {
            return nullptr;
        }

        FString CleanPackagePath = PackagePath;
        if (!CleanPackagePath.StartsWith(TEXT("/Game")))
        {
            CleanPackagePath = TEXT("/Game");
        }

        const FString FullAssetPath = CleanPackagePath / AssetName;
        if (FindObject<UBlueprint>(nullptr, *FullAssetPath))
        {
            return FindObject<UBlueprint>(nullptr, *FullAssetPath);
        }

        UPackage* ExistingPackage = FindPackage(nullptr, *FullAssetPath);
        if (ExistingPackage)
        {
            if (UBlueprint* ExistingBlueprint = FindObject<UBlueprint>(ExistingPackage, *AssetName))
            {
                return ExistingBlueprint;
            }
        }

        UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
            ParentClass,
            CreatePackage(*FullAssetPath),
            *AssetName,
            BPTYPE_Normal,
            UBlueprint::StaticClass(),
            UBlueprintGeneratedClass::StaticClass(),
            FName(TEXT("SquadAIPackFactory")));

        if (!Blueprint || !Blueprint->SimpleConstructionScript)
        {
            return nullptr;
        }

        USCS_Node* RagdollCapsuleNode = nullptr;
        USCS_Node* WeaponMeshNode = nullptr;
        USCS_Node* HealthbarNode = nullptr;
        USCS_Node* StimuliNode = nullptr;
        USCS_Node* HealthNode = nullptr;
        USCS_Node* WeaponAICompNode = nullptr;
        USCS_Node* MeleeNode = nullptr;
        USCS_Node* RagdollNode = nullptr;
        USCS_Node* NoiseEmitterNode = nullptr;
        USCS_Node* PackAutoNode = nullptr;
        USCS_Node* LoadoutBridgeNode = nullptr;
        USCS_Node* ValidationNode = nullptr;
        USCS_Node* LeaderTeamNode = nullptr;

        AddComponentNode(Blueprint, UCapsuleComponent::StaticClass(), TEXT("RagdollCapsule"), RagdollCapsuleNode);
        AddComponentNode(Blueprint, USkeletalMeshComponent::StaticClass(), TEXT("WeaponMesh"), WeaponMeshNode);
        AddComponentNode(Blueprint, UWidgetComponent::StaticClass(), TEXT("HealthBarWidgetComp"), HealthbarNode);
        AddComponentNode(Blueprint, UAIPerceptionStimuliSourceComponent::StaticClass(), TEXT("AIPerceptionStimuliSource"), StimuliNode);
        AddComponentNode(Blueprint, ResolvePreferredComponentClass(TEXT("BPC_HealthAI"), UActorComponent::StaticClass()), TEXT("BPC_HealthAI"), HealthNode);
        AddComponentNode(Blueprint, ResolvePreferredComponentClass(TEXT("BPC_WeaponCharacterAI"), UActorComponent::StaticClass()), TEXT("BPC_WeaponCharacterAI"), WeaponAICompNode);

        // TPWCS docs list BPC_MeleeAI and BPC_RagdollMaster for AI characters.
        // Do not add BPC_MeleeComponentAI or BPC_Ragdoll here: those caused abstract-class load warnings
        // and are not the documented AI character components.
        AddComponentNode(Blueprint, ResolvePreferredComponentClass(TEXT("BPC_MeleeAI"), UActorComponent::StaticClass()), TEXT("BPC_MeleeAI"), MeleeNode);
        AddComponentNode(Blueprint, ResolvePreferredComponentClass(TEXT("BPC_RagdollMaster"), UActorComponent::StaticClass()), TEXT("BPC_RagdollMaster"), RagdollNode);

        AddComponentNode(Blueprint, USquadAIPackAutoCharacterComponent::StaticClass(), TEXT("SquadAIPackAutoCharacterComponent"), PackAutoNode);
        AddComponentNode(Blueprint, USquadAIPackLoadoutBridgeComponent::StaticClass(), TEXT("SquadAIPackLoadoutBridgeComponent"), LoadoutBridgeNode);
        AddComponentNode(Blueprint, USquadAIPackValidationComponent::StaticClass(), TEXT("SquadAIPackValidationComponent"), ValidationNode);

        if (bAddLeaderTeamAutoSetup)
        {
            AddComponentNode(Blueprint, USquadAILeaderTeamAutoSetupComponent::StaticClass(), TEXT("SquadAILeaderTeamAutoSetupComponent"), LeaderTeamNode);
        }

        AttachNodeToParent(Blueprint, WeaponMeshNode, TEXT("CharacterMesh0"));
        AttachNodeToParent(Blueprint, RagdollCapsuleNode, TEXT("CharacterMesh0"));
        AttachNodeToParent(Blueprint, HealthbarNode, TEXT("CharacterMesh0"));

        ConfigureRagdollCapsuleNode(RagdollCapsuleNode);
        ConfigureWeaponMeshNode(WeaponMeshNode);
        ConfigureHealthbarNode(HealthbarNode);
        ConfigureDefaultLoadoutBridgeNode(LoadoutBridgeNode);
        AddTagToComponentNode(WeaponAICompNode, TEXT("TPWCS_BPCWeapon"));

        for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (Node && Node->GetVariableName() == TEXT("CharacterMesh0"))
            {
                ConfigureMeshNode(Node);
                break;
            }
        }

        Blueprint->MarkPackageDirty();
        FAssetRegistryModule::AssetCreated(Blueprint);
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        ConfigureDefaultLoadoutBridgeNode(LoadoutBridgeNode);
        ConfigureGeneratedCharacterDefaults(Blueprint, ParentClass);
        Blueprint->MarkPackageDirty();
        return Blueprint;
    }
#endif
}

UBlueprint* USquadAIBlueprintAssetFactoryLibrary::CreateLeaderPackBlueprintAsset(const FString& PackagePath, const FString& AssetName)
{
#if WITH_EDITOR
    return SquadAIBlueprintAssetFactoryInternal::CreatePackBlueprintAsset(ALeaderCharacter::StaticClass(), PackagePath, AssetName, true);
#else
    return nullptr;
#endif
}

UBlueprint* USquadAIBlueprintAssetFactoryLibrary::CreateAllyPackBlueprintAsset(const FString& PackagePath, const FString& AssetName)
{
#if WITH_EDITOR
    return SquadAIBlueprintAssetFactoryInternal::CreatePackBlueprintAsset(AAllySoldier::StaticClass(), PackagePath, AssetName, false);
#else
    return nullptr;
#endif
}

UBlueprint* USquadAIBlueprintAssetFactoryLibrary::CreateEnemyPackBlueprintAsset(const FString& PackagePath, const FString& AssetName)
{
#if WITH_EDITOR
    return SquadAIBlueprintAssetFactoryInternal::CreatePackBlueprintAsset(AEnemySoldier::StaticClass(), PackagePath, AssetName, false);
#else
    return nullptr;
#endif
}

UBlueprint* USquadAIBlueprintAssetFactoryLibrary::DuplicatePackTemplateAsLeaderBlueprintAsset(const FString& TemplateBlueprintAssetName, const FString& PackagePath, const FString& AssetName)
{
#if WITH_EDITOR
    return SquadAIBlueprintAssetFactoryInternal::DuplicatePackTemplateBlueprintAsset(ALeaderCharacter::StaticClass(), TemplateBlueprintAssetName, PackagePath, AssetName, true);
#else
    return nullptr;
#endif
}

UBlueprint* USquadAIBlueprintAssetFactoryLibrary::DuplicatePackTemplateAsAllyBlueprintAsset(const FString& TemplateBlueprintAssetName, const FString& PackagePath, const FString& AssetName)
{
#if WITH_EDITOR
    return SquadAIBlueprintAssetFactoryInternal::DuplicatePackTemplateBlueprintAsset(AAllySoldier::StaticClass(), TemplateBlueprintAssetName, PackagePath, AssetName, false);
#else
    return nullptr;
#endif
}

UBlueprint* USquadAIBlueprintAssetFactoryLibrary::DuplicatePackTemplateAsEnemyBlueprintAsset(const FString& TemplateBlueprintAssetName, const FString& PackagePath, const FString& AssetName)
{
#if WITH_EDITOR
    return SquadAIBlueprintAssetFactoryInternal::DuplicatePackTemplateBlueprintAsset(AEnemySoldier::StaticClass(), TemplateBlueprintAssetName, PackagePath, AssetName, false);
#else
    return nullptr;
#endif
}
