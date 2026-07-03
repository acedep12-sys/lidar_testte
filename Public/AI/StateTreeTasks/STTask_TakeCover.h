// STTask_TakeCover.h — StateTree Task: async cover query → move to cover → crouch
#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STTask_TakeCover.generated.h"

USTRUCT(meta = (DisplayName = "Take Cover"))
struct SQUADAI_API FSTTask_TakeCover : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FSTTask_TakeCover()
	{
		bShouldCallTick = true;
	}

	using FInstanceDataType = FSTTask_TakeCoverData;

	UPROPERTY(EditAnywhere, Category = "Cover")
	float SearchRadius = 2500.f;

	UPROPERTY(EditAnywhere, Category = "Cover")
	float AcceptanceRadius = 600.f;

	// If the procedural cover scanner cannot find a real cover point quickly, move to
	// a tactical fallback position away/lateral from the threat instead of freezing.
	UPROPERTY(EditAnywhere, Category = "Cover")
	bool bUseFallbackRepositionIfNoCover = true;

	virtual const UStruct* GetInstanceDataType() const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FSTTask_TakeCoverData
{
	GENERATED_BODY()

	enum class EPhase : uint8 { WaitingForResult, Moving, Arrived };

	EPhase Phase = EPhase::WaitingForResult;

	UPROPERTY()
	float WaitTimer = 0.f;

	UPROPERTY()
	FVector MoveGoal = FVector::ZeroVector;

	UPROPERTY()
	bool bHasMoveGoal = false;

	UPROPERTY()
	bool bUsingRealCover = false;

	UPROPERTY()
	float LastMoveRequestTime = -1000.f;

	// Cover reservation info (for cleanup on exit)
	UPROPERTY()
	FIntPoint ReservedChunkKey = FIntPoint::ZeroValue;

	UPROPERTY()
	int32 ReservedLocalID = -1;

	UPROPERTY()
	bool bReserved = false;
};
