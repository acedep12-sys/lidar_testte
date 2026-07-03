#pragma once

#include "CoreMinimal.h"
#include "SquadLeaderTypes.generated.h"

UENUM(BlueprintType)
enum class ESquadAnimationMode : uint8
{
	SquadAIClassic UMETA(DisplayName = "SquadAI Classic"),
	PackDriven UMETA(DisplayName = "Pack Driven")
};

UENUM(BlueprintType)
enum class ESquadCombatPresentationState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Moving UMETA(DisplayName = "Moving"),
	Aiming UMETA(DisplayName = "Aiming"),
	Firing UMETA(DisplayName = "Firing"),
	Reloading UMETA(DisplayName = "Reloading"),
	Dead UMETA(DisplayName = "Dead")
};

USTRUCT(BlueprintType)
struct FLeaderQuestProgressSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Quest")
	bool bQuestActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Quest")
	bool bReachedCurrentWaypoint = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Quest")
	bool bCurrentAreaClear = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Quest")
	bool bQuestComplete = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Quest")
	int32 CurrentWaypointIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Quest")
	FVector CurrentWaypointLocation = FVector::ZeroVector;
};
