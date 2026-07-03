// QuestObjective.cpp — The Direct Command Injector
#include "Quest/QuestObjective.h"
#include "Quest/QuestRegistry.h"
#include "Quest/QuestZone.h"
#include "AI/SoldierAIController.h"
#include "AI/SquadAICommandTypes.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

void UQuestObjective_AutoZone::OnStart(UWorld* World)
{
	Super::OnStart(World);
	if (!World) return;
	UQuestRegistry* Reg = World->GetSubsystem<UQuestRegistry>();
	if (!Reg) return;
	
	AActor* Found = Reg->Find(ZoneTag);
	CachedZone = Cast<AQuestZone>(Found);
	FVector GoalLoc = Found ? Found->GetActorLocation() : FVector::ZeroVector;

	// SET THE GPS BEACON
	Reg->CurrentActiveGoal = GoalLoc;

	// ---- DIRECT COMMAND: Force-push the goal to allied AI brains only ----
	for (TActorIterator<ASoldierAIController> It(World); It; ++It)
	{
		ASoldierAIController* AIC = *It;
		if (AIC && AIC->GetGenericTeamId().GetId() == 0)
		{
			FSquadAIMoveCommand MoveCommand;
			MoveCommand.Mode = ESquadAIMoveMode::QuestGoal;
			MoveCommand.GoalLocation = GoalLoc;
			MoveCommand.AcceptanceRadius = 500.f;
			MoveCommand.bAllowPartialPath = true;
			AIC->SetMoveCommand(MoveCommand);
		}
	}

	ResolveMode(World);
}

void UQuestObjective_AutoZone::Tick(float DeltaTime, UWorld* World)
{
	if (State != EObjectiveState::Active) return;
	
	UQuestRegistry* Reg = World->GetSubsystem<UQuestRegistry>();
	if (!Reg) return;

	if (CachedZone.IsValid() && CachedZone->IsPlayerInZone())
	{
		State = EObjectiveState::Succeeded;
		return;
	}

	// Fallback: allied leader / squad arrival also counts.
	for (TActorIterator<ASoldierAIController> It(World); It; ++It)
	{
		ASoldierAIController* AIC = *It;
		if (AIC && AIC->GetPawn() && AIC->GetGenericTeamId().GetId() == 0)
		{
			const float DistanceToGoal = FVector::Dist(AIC->GetPawn()->GetActorLocation(), AIC->GetEffectiveMoveGoal());
			if (DistanceToGoal < 600.f)
			{
				State = EObjectiveState::Succeeded;
				break;
			}
		}
	}
}

void UQuestObjective_AutoZone::OnFinish() { Super::OnFinish(); }
void UQuestObjective_AutoZone::ResolveMode(UWorld* World) { ResolvedMode = EObjectiveMode::Reach; }
