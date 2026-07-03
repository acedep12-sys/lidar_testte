// SquadAITuning.h — Every gameplay number in ONE place
// Accessible via: Project Settings → Plugins → Squad AI Tuning
// Or via: GetDefault<USquadAITuning>() (zero-cost CDO read)
// Or via: USquadAITuning::Get()
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SquadTypes.h"
#include "SquadAITuning.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Squad AI Tuning"))
class SQUADAI_API USquadAITuning : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USquadAITuning();

	static const USquadAITuning* Get()
	{
		return GetDefault<USquadAITuning>();
	}

	virtual FName GetCategoryName() const override { return FName("Plugins"); }

	// =====================================================================
	//  COVER SCANNER
	// =====================================================================

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	float ChunkSizeXY = 1000.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	float ChunkSizeZ = 500.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	int32 SamplesPerChunkAxis = 6;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	float ChunkRevalidationSeconds = 30.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	int32 MaxChunkScansPerFrame = 2;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	int32 MaxQueriesPerFrame = 16;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	int32 AsyncTraceBudgetPerFrame = 96;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	float CrouchHeight = 95.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	float StandHeight = 180.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	float MinCoverThickness = 25.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	float NavmeshProjectExtent = 150.f;

	// Minimum tactical cover search radius used by TakeCover even if a StateTree asset
	// has an older/smaller value serialized.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	float TacticalCoverSearchRadius = 3500.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	float CoverQueryPartialResolveSeconds = 0.6f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	float CoverQueryTimeoutSeconds = 3.0f;

	// Ground scan height around the player/leader altitude. Useful for large landscapes
	// whose absolute Z can be 6000-22000 while local terrain variation is modest.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	float CoverGroundTraceHalfHeight = 2500.f;

	// Safety fallback if the local-height trace misses because the player is far above/below
	// the chunk being scanned.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cover Scanner")
	bool bCoverUseLargeHeightFallbackTrace = true;

	// =====================================================================
	//  COMBAT — Weapon
	// =====================================================================

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	float DefaultDamage = 15.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	float DefaultFireRate = 8.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	float DefaultBaseSpread = 1.5f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	float DefaultSpreadPerShot = 0.4f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	float DefaultSpreadDecay = 4.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	float DefaultMaxRange = 10000.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	float DefaultFalloffStart = 2500.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	float DefaultMinDamageMultiplier = 0.35f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	float SuppressionRadius = 250.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	float SuppressionAmountPerBullet = 0.35f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	float DamageMultiplier = 1.f;

	// =====================================================================
	//  COMBAT — Health
	// =====================================================================

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Health")
	float DefaultMaxHealth = 100.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Health")
	float InvincibleMinHealth = 1.f;

	// =====================================================================
	//  COMBAT — Aim
	// =====================================================================

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim")
	float AimBodyYawRate = 360.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim")
	float AimSpringStiffness = 14.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim")
	float AimSpringDampingRatio = 1.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim")
	float AimSpineConeDegrees = 75.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Aim")
	float AimMaxErrorDegrees = 2.5f;

	// =====================================================================
	//  PERCEPTION
	// =====================================================================

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Perception")
	float SightRadius = 3500.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Perception")
	float LoseSightRadius = 4200.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Perception")
	float PeripheralVisionDegrees = 70.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Perception")
	float HearingRange = 2500.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Perception")
	float MemoryDecayPerSecond = 0.2f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Perception")
	float SuppressionDecayPerSecond = 0.4f;

	// Registry-based combat detection range. This complements UE perception and is exposed
	// in Project Settings so large outdoor maps can detect threats at realistic distances.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Detection")
	float CombatDetectionRange = 15000.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Detection")
	float CombatCloseRangeOverride = 1800.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Detection")
	float MaxEngageRange = 8000.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Detection")
	float DirectFallbackFireRange = 8000.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat|Detection")
	bool bAllowDirectFallbackFire = true;

	// =====================================================================
	//  FACTION — Allies
	// =====================================================================

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Faction|Allies")
	float AllyAccuracyMultiplier = 0.25f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Faction|Allies")
	float AllyDefaultPowerLevel = 0.25f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Faction|Allies")
	int32 AllyCount = 4;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Faction|Allies")
	bool bAlliesInvincible = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Faction|Allies")
	float AllyFormationSpacing = 200.f;

	// =====================================================================
	//  FACTION — Enemies
	// =====================================================================

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Faction|Enemies")
	float EnemyAccuracyMultiplier = 0.65f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Faction|Enemies")
	float EnemyDefaultAggression = 0.4f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Faction|Enemies")
	float EnemySuppressionThreshold = 0.6f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Faction|Enemies")
	int32 MaxVisibleEnemies = 50;

	// =====================================================================
	//  LEADER
	// =====================================================================

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Leader")
	float LeaderWaitForPlayerRadius = 1500.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Leader")
	float LeaderFullSpeedRadius = 800.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Leader")
	float LeaderNormalSpeed = 400.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Leader")
	float LeaderWaitingSpeed = 120.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Leader")
	bool bLeaderInvincible = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Leader")
	float NavInvokerRadius = 5000.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Leader")
	float NavInvokerRemovalRadius = 10000.f;

	// =====================================================================
	//  LOD / PERFORMANCE
	// =====================================================================

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Performance")
	float SignificanceUpdateInterval = 0.25f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Performance")
	int32 MaxHighFidelityAgents = 16;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Performance")
	float SignificanceDormantDistance = 8000.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Performance")
	float RegistryCellSize = 4000.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Performance")
	float RegistryRebucketInterval = 0.2f;

	// =====================================================================
	//  DIFFICULTY
	// =====================================================================

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float AllyPower_Easy = 0.45f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float AllyPower_Normal = 0.25f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float AllyPower_Hard = 0.15f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float AllyPower_Lethal = 0.05f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float EnemyDamage_Easy = 0.70f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float EnemyDamage_Normal = 1.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float EnemyDamage_Hard = 1.30f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float EnemyDamage_Lethal = 1.60f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float EnemyAccuracy_Easy = 0.40f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float EnemyAccuracy_Normal = 0.65f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float EnemyAccuracy_Hard = 0.80f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float EnemyAccuracy_Lethal = 0.92f;

	// =====================================================================
	//  QUEST
	// =====================================================================

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Quest")
	float QuestTickRate = 0.25f;   // 4 Hz

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Quest")
	float CorpseLingerSeconds = 8.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Quest")
	int32 EnemyPoolPrewarmCount = 30;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Quest")
	float SpawnPointNavProjectExtent = 120.f;
};
