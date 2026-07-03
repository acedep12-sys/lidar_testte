#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SquadAIPackBridgeLibrary.generated.h"

class ASoldierCharacter;
class ALeaderCharacter;
class AAllySoldier;
class UMeshComponent;

UCLASS()
class SQUADAI_API USquadAIPackBridgeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static bool AutoSetupSquadAIPackCharacter(AActor* Actor, UMeshComponent* WeaponMeshComponent, bool bEnableLeftHandIK = false, FName AttachSocketName = FName("ik_hand_gun"), FName LeftHandIKSocketName = FName("LeftHandIK"), FName MuzzleSocketName = FName("Muzzle"));

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static bool AutoSetupLeaderForPack(AActor* Actor, UMeshComponent* WeaponMeshComponent);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static bool AutoSetupAllyForPack(AActor* Actor, UMeshComponent* WeaponMeshComponent);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static bool AutoSetupLeaderAndAlliesForPack(AActor* LeaderActor, UMeshComponent* LeaderWeaponMeshComponent, const TArray<AActor*>& AllyActors, const TArray<UMeshComponent*>& AllyWeaponMeshComponents);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static bool AutoAssignLeaderToAllies(AActor* LeaderActor, const TArray<AActor*>& AllyActors);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static int32 AutoAssignFormationSlotsToAllies(const TArray<AActor*>& AllyActors, int32 StartingSlot = 0);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static bool AutoSetupLeaderTeamForPack(AActor* LeaderActor, UMeshComponent* LeaderWeaponMeshComponent, const TArray<AActor*>& AllyActors, const TArray<UMeshComponent*>& AllyWeaponMeshComponents, int32 StartingFormationSlot = 0);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static bool AutoSetupSingleAllyForLeaderTeam(AActor* LeaderActor, AActor* AllyActor, UMeshComponent* AllyWeaponMeshComponent, int32 FormationSlot);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static bool AutoConfigureExistingLeaderTeam(AActor* LeaderActor, const TArray<AActor*>& AllyActors, int32 StartingFormationSlot = 0);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static bool AutoBuildLeaderWaypointsFromQuest(AActor* LeaderActor, bool bAutoBeginQuest = true);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static bool AutoSetupExistingLeaderTeamFromQuest(AActor* LeaderActor, const TArray<AActor*>& AllyActors, int32 StartingFormationSlot = 0, bool bAutoBuildWaypointsFromQuest = true, bool bAutoBeginQuest = true);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static bool AutoSetupSingleLeaderTeamMember(AActor* LeaderActor, AActor* TeamMemberActor, UMeshComponent* TeamMemberWeaponMeshComponent, int32 FormationSlot = 0, bool bTreatAsLeader = false);

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Pack Bridge")
	static bool ForwardPackDamageToSquadAI(AActor* HitActor, float DamageAmount, AActor* DamageCauser, AController* InstigatorController, TSubclassOf<UDamageType> DamageTypeClass);
};
