#pragma once

#include "CoreMinimal.h"
#include "SquadAICommandTypes.generated.h"

UENUM(BlueprintType)
enum class ESquadAIMoveMode : uint8
{
	None UMETA(DisplayName = "None"),
	QuestGoal UMETA(DisplayName = "Quest Goal"),
	FollowLeader UMETA(DisplayName = "Follow Leader"),
	CombatCover UMETA(DisplayName = "Combat Cover"),
	HoldPosition UMETA(DisplayName = "Hold Position"),
};

UENUM(BlueprintType)
enum class ESquadAIAimMode : uint8
{
	None UMETA(DisplayName = "None"),
	CombatTarget UMETA(DisplayName = "Combat Target"),
	InvestigateLocation UMETA(DisplayName = "Investigate Location"),
};

USTRUCT(BlueprintType)
struct FSquadAIMoveCommand
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SquadAI")
	ESquadAIMoveMode Mode = ESquadAIMoveMode::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SquadAI")
	FVector GoalLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SquadAI")
	float AcceptanceRadius = 500.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SquadAI")
	bool bAllowPartialPath = true;

	bool IsValid() const
	{
		return Mode != ESquadAIMoveMode::None && !GoalLocation.IsNearlyZero();
	}
};

USTRUCT(BlueprintType)
struct FSquadAIAimCommand
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SquadAI")
	ESquadAIAimMode Mode = ESquadAIAimMode::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SquadAI")
	FVector AimLocation = FVector::ZeroVector;

	// Optional real actor target. This matters for external Blueprint combat packs like TPWCS,
	// which may use an aimedActor / aiming-info interface rather than only a world location.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SquadAI")
	TObjectPtr<AActor> TargetActor = nullptr;

	bool IsValid() const
	{
		return Mode != ESquadAIAimMode::None && !AimLocation.IsNearlyZero();
	}
};
