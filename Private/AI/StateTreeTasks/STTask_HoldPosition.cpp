// STTask_HoldPosition.cpp
#include "AI/StateTreeTasks/STTask_HoldPosition.h"
#include "AI/SoldierAIController.h"
#include "AI/SquadAICommandTypes.h"
#include "Characters/SoldierCharacter.h"
#include "StateTreeExecutionContext.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"

const UStruct* FSTTask_HoldPosition::GetInstanceDataType() const { return FSTTask_HoldPositionData::StaticStruct(); }

EStateTreeRunStatus FSTTask_HoldPosition::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const {
	auto& D = Context.GetInstanceData<FSTTask_HoldPositionData>(*this);
	D.Timer = 0.f; return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_HoldPosition::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const {
	auto& D = Context.GetInstanceData<FSTTask_HoldPositionData>(*this);
	ASoldierAIController* AI = Cast<ASoldierAIController>(Context.GetOwner());
	if (AI && AI->GetPawn()) {
		float Confidence = 0.f;
		if (AActor* Threat = AI->GetPrimaryThreat(Confidence)) {
			AI->SetCombatAimTarget(Threat->GetActorLocation() + FVector(0.f, 0.f, 70.f), ESquadAIAimMode::CombatTarget, Threat);
		}
		else
		{
			AI->StopCombatPresentation();
		}
	}
	if (HoldDuration > 0.f) {
		D.Timer += DeltaTime;
		if (D.Timer >= HoldDuration) return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}
