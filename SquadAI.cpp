// SquadAI.cpp — Global Team Attitude Solver
#include "SquadAI.h"
#include "GenericTeamAgentInterface.h"

#define LOCTEXT_NAMESPACE "FSquadAIModule"

// Global solver: Team 0 (Allies) and Team 1 (Enemies) are Hostile.
static ETeamAttitude::Type SquadTeamAttitudeSolver(FGenericTeamId A, FGenericTeamId B)
{
	if (A == FGenericTeamId::NoTeam || B == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	return (A == B) ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
}

void FSquadAIModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("SquadAI module loaded"));

	// Assign the global attitude solver
	FGenericTeamId::SetAttitudeSolver(&SquadTeamAttitudeSolver);
}

void FSquadAIModule::ShutdownModule()
{
	FGenericTeamId::SetAttitudeSolver(nullptr);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_PRIMARY_GAME_MODULE(FSquadAIModule, SquadAI, "SquadAI");
