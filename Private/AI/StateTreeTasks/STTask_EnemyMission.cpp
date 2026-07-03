// STTask_EnemyMission.cpp
#include "AI/StateTreeTasks/STTask_EnemyMission.h"
#include "AI/SoldierAIController.h"
#include "Quest/QuestRegistry.h"
#include "Quest/QuestZone.h"
#include "StateTreeExecutionContext.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

const UStruct* FSTTask_EnemyMission::GetInstanceDataType() const { return FSTTask_EnemyMissionData::StaticStruct(); }

static ASoldierAIController* GetEnemyMissionController(FStateTreeExecutionContext& Context)
{
	if (ASoldierAIController* AIC = Cast<ASoldierAIController>(Context.GetOwner()))
	{
		return AIC;
	}

	if (APawn* Pawn = Cast<APawn>(Context.GetOwner()))
	{
		return Cast<ASoldierAIController>(Pawn->GetController());
	}

	return nullptr;
}

EStateTreeRunStatus FSTTask_EnemyMission::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const {
	auto& D = Context.GetInstanceData<FSTTask_EnemyMissionData>(*this);
	D.LastGoalLoc = FVector::ZeroVector;
	D.TimeSinceRepath = 1.0f;
	D.bMoveRequested = false;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_EnemyMission::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const {
	auto& D = Context.GetInstanceData<FSTTask_EnemyMissionData>(*this);
	ASoldierAIController* AI = GetEnemyMissionController(Context);
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;
	
	D.TimeSinceRepath += DeltaTime;
	FVector GoalLoc = AI->GetEffectiveMoveGoal();

	// Last-resort fallback: if the mission did not push a goal yet, move to the first
	// QuestZone in the world. This prevents the StateTree from showing the mission task
	// as Running while there is no actual movement goal.
	if (GoalLoc.IsNearlyZero()) {
		for (TActorIterator<AQuestZone> It(AI->GetWorld()); It; ++It) {
			GoalLoc = It->GetActorLocation();
			break;
		}
	}

	if (GoalLoc.IsNearlyZero()) {
		if (D.TimeSinceRepath >= 2.0f) {
			UE_LOG(LogTemp, Warning, TEXT("EnemyMission: %s has no goal. CurrentGoalLocation and QuestRegistry::CurrentActiveGoal are zero, and no QuestZone was found."),
				*GetNameSafe(AI->GetPawn()));
			D.TimeSinceRepath = 0.f;
		}
		return EStateTreeRunStatus::Running;
	}

	UPathFollowingComponent* PFC = AI->GetPathFollowingComponent();
	const float AcceptanceRadius = 500.f;
	const float DistToGoal = FVector::Dist(AI->GetPawn()->GetActorLocation(), GoalLoc);
	const bool bSameGoal = FVector::DistSquared(GoalLoc, AI->LastMissionMoveGoal) < 10000.f;
	const bool bAlreadyMoving = PFC && PFC->GetStatus() == EPathFollowingStatus::Moving;
	const bool bAlreadyAtGoal = bSameGoal && DistToGoal <= AcceptanceRadius + 100.f;
	const float Now = AI->GetWorld()->GetTimeSeconds();

	// If the previous request is still active and the goal has not changed, leave it alone.
	// Use controller-level state because StateTree task instance data may be re-entered/reset
	// depending on how the StateTree is authored.
	if (bSameGoal && (bAlreadyMoving || bAlreadyAtGoal)) return EStateTreeRunStatus::Running;

	// Avoid spamming/restarting a path every StateTree tick. Repeated StopMovement/MoveTo
	// can keep the pawn visually frozen even while PathFollowing briefly reports Moving.
	if ((Now - AI->LastMissionMoveRequestTime) < 1.0f && bSameGoal) return EStateTreeRunStatus::Running;

	// Use the AIController's native move request instead of UAITask_MoveTo here.
	// It makes failures visible immediately and directly drives PathFollowing.
	// IMPORTANT: do not call StopMovement before re-requesting the same goal; that can
	// cancel velocity every tick if the StateTree re-enters this task.
	const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(
		GoalLoc,
		AcceptanceRadius,
		true,   // bStopOnOverlap
		true,   // bUsePathfinding
		true,   // bProjectDestinationToNavigation
		false,  // bCanStrafe
		nullptr,
		true    // bAllowPartialPath
	);

	D.LastGoalLoc = GoalLoc;
	D.TimeSinceRepath = 0.f;
	D.bMoveRequested = (Result == EPathFollowingRequestResult::RequestSuccessful || Result == EPathFollowingRequestResult::AlreadyAtGoal);
	AI->LastMissionMoveGoal = GoalLoc;
	AI->LastMissionMoveRequestTime = Now;

	const TCHAR* ResultText = TEXT("Unknown");
	switch (Result)
	{
	case EPathFollowingRequestResult::Failed: ResultText = TEXT("Failed"); break;
	case EPathFollowingRequestResult::AlreadyAtGoal: ResultText = TEXT("AlreadyAtGoal"); break;
	case EPathFollowingRequestResult::RequestSuccessful: ResultText = TEXT("RequestSuccessful"); break;
	}

	UE_LOG(LogTemp, Warning, TEXT("EnemyMission: %s MoveTo %s -> %s. Dist=%.0f PathStatus=%s"),
		*GetNameSafe(AI->GetPawn()), *GoalLoc.ToString(), ResultText,
		FVector::Dist(AI->GetPawn()->GetActorLocation(), GoalLoc),
		PFC ? *UEnum::GetValueAsString(PFC->GetStatus()) : TEXT("NoPathFollowingComponent"));

	return EStateTreeRunStatus::Running;
}

void FSTTask_EnemyMission::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const {
	auto& D = Context.GetInstanceData<FSTTask_EnemyMissionData>(*this);
	D.bMoveRequested = false;

	// Do not StopMovement here. If this task/state is re-entered often, stopping on exit
	// will cancel the move every frame and the pawn will appear frozen while MoveTo logs
	// RequestSuccessful/Moving repeatedly. Other combat/cover tasks can issue their own
	// movement or explicit stop when they truly need to take over.
}
