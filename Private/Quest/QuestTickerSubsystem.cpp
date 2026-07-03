// QuestTickerSubsystem.cpp — 4Hz tick for active missions only
#include "Quest/QuestTickerSubsystem.h"
#include "Quest/QuestDirector.h"
#include "Quest/QuestMission.h"
#include "SquadAITuning.h"
#include "EngineUtils.h"

void UQuestTickerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bQuestMapConfirmed = false;
	bQuestMapEnabled = false;
}

void UQuestTickerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

bool UQuestTickerSubsystem::IsTickable() const
{
	// If the current world is not using the quest system, stay completely dormant.
	if (bQuestMapConfirmed && !bQuestMapEnabled)
	{
		return false;
	}

	// Tick while discovering whether this map contains a QuestDirector.
	if (!Director.IsValid()) return true;

	AQuestMission* Current = Director->GetCurrentMission();
	return Current && Current->MissionState == EMissionState::Active;
}

void UQuestTickerSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Director.IsValid())
	{
		FindDirector();
		return;
	}

	AQuestMission* Current = Director->GetCurrentMission();
	if (!Current) return;

	if (Current->MissionState == EMissionState::Active)
	{
		Current->TickMission(DeltaTime);
		BroadcastHUD(Current);

		if (Current->MissionState == EMissionState::Succeeded)
		{
			Director->OnMissionCompleted(Current);
		}
	}
}

void UQuestTickerSubsystem::FindDirector()
{
	AQuestDirector* FoundDirector = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (AQuestDirector* Found = Cast<AQuestDirector>(*It))
		{
			FoundDirector = Found;
			break;
		}
	}

	bQuestMapConfirmed = true;
	bQuestMapEnabled = (FoundDirector != nullptr);
	Director = FoundDirector;
}

void UQuestTickerSubsystem::SetDirector(AQuestDirector* Dir)
{
	Director = Dir;
	bQuestMapConfirmed = true;
	bQuestMapEnabled = (Dir != nullptr);
}

void UQuestTickerSubsystem::BroadcastHUD(AQuestMission* Mission)
{
	if (!Mission) return;

	FQuestHudSnapshot Snap = Mission->GetHudSnapshot();
	OnHudUpdated.Broadcast(Snap);
}
