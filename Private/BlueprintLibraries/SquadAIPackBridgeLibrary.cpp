#include "BlueprintLibraries/SquadAIPackBridgeLibrary.h"
#include "Characters/SoldierCharacter.h"
#include "Characters/LeaderCharacter.h"
#include "Characters/AllySoldier.h"
#include "Components/MeshComponent.h"
#include "Quest/QuestDirector.h"
#include "Quest/QuestMission.h"
#include "Quest/QuestObjective.h"
#include "Quest/QuestRegistry.h"
#include "EngineUtils.h"

bool USquadAIPackBridgeLibrary::AutoSetupSquadAIPackCharacter(AActor* Actor, UMeshComponent* WeaponMeshComponent, bool bEnableLeftHandIK, FName AttachSocketName, FName LeftHandIKSocketName, FName MuzzleSocketName)
{
	ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(Actor);
	if (!Soldier)
	{
		return false;
	}

	Soldier->SetUsePackPresentationMode(true);
	Soldier->SetEquippedWeaponMesh(WeaponMeshComponent);
	Soldier->WeaponAttachSocketName = AttachSocketName;
	Soldier->WeaponLeftHandIKSocketName = LeftHandIKSocketName;
	Soldier->WeaponMuzzleSocketName = MuzzleSocketName;
	// In pack presentation mode we intentionally do not override hand IK setup.
	(void)bEnableLeftHandIK;
	return true;
}

bool USquadAIPackBridgeLibrary::AutoSetupLeaderForPack(AActor* Actor, UMeshComponent* WeaponMeshComponent)
{
	ALeaderCharacter* Leader = Cast<ALeaderCharacter>(Actor);
	if (!Leader)
	{
		return false;
	}

	Leader->AnimationMode = ESquadAnimationMode::PackDriven;
	return AutoSetupSquadAIPackCharacter(Leader, WeaponMeshComponent, false, FName("ik_hand_gun"), FName("LeftHandIK"), FName("Muzzle"));
}

bool USquadAIPackBridgeLibrary::AutoSetupAllyForPack(AActor* Actor, UMeshComponent* WeaponMeshComponent)
{
	AAllySoldier* Ally = Cast<AAllySoldier>(Actor);
	if (!Ally)
		return false;

	Ally->AnimationMode = ESquadAnimationMode::PackDriven;
	return AutoSetupSquadAIPackCharacter(Ally, WeaponMeshComponent, false, FName("ik_hand_gun"), FName("LeftHandIK"), FName("Muzzle"));
}

bool USquadAIPackBridgeLibrary::AutoSetupLeaderAndAlliesForPack(AActor* LeaderActor, UMeshComponent* LeaderWeaponMeshComponent, const TArray<AActor*>& AllyActors, const TArray<UMeshComponent*>& AllyWeaponMeshComponents)
{
	bool bAllSucceeded = AutoSetupLeaderForPack(LeaderActor, LeaderWeaponMeshComponent);

	const int32 PairCount = FMath::Min(AllyActors.Num(), AllyWeaponMeshComponents.Num());
	for (int32 Index = 0; Index < PairCount; ++Index)
	{
		bAllSucceeded = AutoSetupAllyForPack(AllyActors[Index], AllyWeaponMeshComponents[Index]) && bAllSucceeded;
	}

	if (AllyActors.Num() != AllyWeaponMeshComponents.Num())
	{
		bAllSucceeded = false;
	}

	bAllSucceeded = AutoAssignLeaderToAllies(LeaderActor, AllyActors) && bAllSucceeded;
	return bAllSucceeded;
}

bool USquadAIPackBridgeLibrary::AutoAssignLeaderToAllies(AActor* LeaderActor, const TArray<AActor*>& AllyActors)
{
	ALeaderCharacter* Leader = Cast<ALeaderCharacter>(LeaderActor);
	if (!Leader)
	{
		return false;
	}

	bool bAllSucceeded = true;
	for (AActor* AllyActor : AllyActors)
	{
		AAllySoldier* Ally = Cast<AAllySoldier>(AllyActor);
		if (!Ally)
		{
			bAllSucceeded = false;
			continue;
		}

		Ally->LeaderOverride = Leader;
	}

	return bAllSucceeded;
}

int32 USquadAIPackBridgeLibrary::AutoAssignFormationSlotsToAllies(const TArray<AActor*>& AllyActors, int32 StartingSlot)
{
	int32 AssignedCount = 0;
	for (int32 Index = 0; Index < AllyActors.Num(); ++Index)
	{
		if (AAllySoldier* Ally = Cast<AAllySoldier>(AllyActors[Index]))
		{
			Ally->FormationSlot = StartingSlot + AssignedCount;
			++AssignedCount;
		}
	}
	return AssignedCount;
}

bool USquadAIPackBridgeLibrary::AutoSetupLeaderTeamForPack(AActor* LeaderActor, UMeshComponent* LeaderWeaponMeshComponent, const TArray<AActor*>& AllyActors, const TArray<UMeshComponent*>& AllyWeaponMeshComponents, int32 StartingFormationSlot)
{
	bool bSucceeded = AutoSetupLeaderAndAlliesForPack(LeaderActor, LeaderWeaponMeshComponent, AllyActors, AllyWeaponMeshComponents);
	const int32 AssignedCount = AutoAssignFormationSlotsToAllies(AllyActors, StartingFormationSlot);
	if (AssignedCount != AllyActors.Num())
	{
		bSucceeded = false;
	}
	return bSucceeded;
}

bool USquadAIPackBridgeLibrary::AutoSetupSingleAllyForLeaderTeam(AActor* LeaderActor, AActor* AllyActor, UMeshComponent* AllyWeaponMeshComponent, int32 FormationSlot)
{
	ALeaderCharacter* Leader = Cast<ALeaderCharacter>(LeaderActor);
	AAllySoldier* Ally = Cast<AAllySoldier>(AllyActor);
	if (!Leader || !Ally)
	{
		return false;
	}

	const bool bPackSetupSucceeded = AutoSetupAllyForPack(Ally, AllyWeaponMeshComponent);
	Ally->LeaderOverride = Leader;
	Ally->FormationSlot = FormationSlot;
	return bPackSetupSucceeded;
}

bool USquadAIPackBridgeLibrary::AutoConfigureExistingLeaderTeam(AActor* LeaderActor, const TArray<AActor*>& AllyActors, int32 StartingFormationSlot)
{
	const bool bLeaderAssigned = AutoAssignLeaderToAllies(LeaderActor, AllyActors);
	const int32 AssignedCount = AutoAssignFormationSlotsToAllies(AllyActors, StartingFormationSlot);
	return bLeaderAssigned && AssignedCount == AllyActors.Num();
}

static void GatherWaypointsFromObjective(UQuestObjective* Objective, UQuestRegistry* Registry, TArray<FQuestWaypoint>& OutWaypoints)
{
	if (!Objective || !Registry)
	{
		return;
	}

	if (UQuestObjective_AutoZone* AutoZone = Cast<UQuestObjective_AutoZone>(Objective))
	{
		if (AActor* ZoneActor = Registry->Find(AutoZone->ZoneTag))
		{
			FQuestWaypoint Waypoint;
			Waypoint.Location = ZoneActor->GetActorLocation();
			Waypoint.AcceptanceRadius = 500.f;
			Waypoint.bRequireAreaClear = AutoZone->Mode != EObjectiveMode::Reach;
			Waypoint.LingerSeconds = 1.f;
			Waypoint.AreaClearRadius = 1600.f;
			OutWaypoints.Add(Waypoint);
		}
	}
}

bool USquadAIPackBridgeLibrary::AutoBuildLeaderWaypointsFromQuest(AActor* LeaderActor, bool bAutoBeginQuest)
{
	ALeaderCharacter* Leader = Cast<ALeaderCharacter>(LeaderActor);
	if (!Leader || !Leader->GetWorld())
	{
		return false;
	}

	UWorld* World = Leader->GetWorld();
	UQuestRegistry* Registry = World ? World->GetSubsystem<UQuestRegistry>() : nullptr;
	if (!Registry)
	{
		return false;
	}

	AQuestDirector* Director = nullptr;
	for (TActorIterator<AQuestDirector> It(World); It; ++It)
	{
		Director = *It;
		break;
	}

	if (!Director)
	{
		return false;
	}

	TArray<FQuestWaypoint> BuiltWaypoints;
	for (AQuestMission* Mission : Director->Missions)
	{
		if (!Mission)
		{
			continue;
		}

		for (UQuestObjective* Objective : Mission->Objectives)
		{
			GatherWaypointsFromObjective(Objective, Registry, BuiltWaypoints);
		}
	}

	if (BuiltWaypoints.Num() == 0)
	{
		return false;
	}

	Leader->SetQuestWaypoints(BuiltWaypoints);
	if (bAutoBeginQuest)
	{
		Leader->BeginQuest();
	}
	return true;
}

bool USquadAIPackBridgeLibrary::AutoSetupExistingLeaderTeamFromQuest(AActor* LeaderActor, const TArray<AActor*>& AllyActors, int32 StartingFormationSlot, bool bAutoBuildWaypointsFromQuest, bool bAutoBeginQuest)
{
	bool bSucceeded = AutoConfigureExistingLeaderTeam(LeaderActor, AllyActors, StartingFormationSlot);
	if (bAutoBuildWaypointsFromQuest)
	{
		bSucceeded = AutoBuildLeaderWaypointsFromQuest(LeaderActor, bAutoBeginQuest) && bSucceeded;
	}
	return bSucceeded;
}

bool USquadAIPackBridgeLibrary::AutoSetupSingleLeaderTeamMember(AActor* LeaderActor, AActor* TeamMemberActor, UMeshComponent* TeamMemberWeaponMeshComponent, int32 FormationSlot, bool bTreatAsLeader)
{
	if (bTreatAsLeader)
	{
		return AutoSetupLeaderForPack(TeamMemberActor, TeamMemberWeaponMeshComponent);
	}

	return AutoSetupSingleAllyForLeaderTeam(LeaderActor, TeamMemberActor, TeamMemberWeaponMeshComponent, FormationSlot);
}

bool USquadAIPackBridgeLibrary::ForwardPackDamageToSquadAI(AActor* HitActor, float DamageAmount, AActor* DamageCauser, AController* InstigatorController, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(HitActor))
	{
		Soldier->NotifyPackDamage(DamageAmount, DamageCauser, InstigatorController, DamageTypeClass);
		return true;
	}
	return false;
}
