// Fill out your copyright notice in the Description page of Project Settings.


#include "KBGameModeSurvival.h"
#include "KBGameState.h"
#include "Actors/KBGrid.h"

// game starts when at least 1 player is ready, countdown, let other players ready up too
// kind of the same as versus, but no walls, everyone is in the same arena
// boxes spawn faster and faster, and also crush players
// i'll need to make that thing where a player dies if they're trapped (can't move any nearby box)

// need to make sure that a player cant block 2 spaces from spawning a box and thus prolong the game forever

void AKBGameModeSurvival::StartBoxTimer()
{
	AdjustedSpawnRate = 3.f;

	// Don't loop so we can restart it with a lower time
	GetWorld()->GetTimerManager().SetTimer(BoxSpawnTimer, this, &AKBGameModeSurvival::SpawnBoxSurvival, 3.f, false);
}

void AKBGameModeSurvival::SpawnBoxSurvival()
{
	AKBGameState * gameState = GetGameState<AKBGameState>();
	if (!IsValid(gameState) || !gameState->GetGrid())
	{
		GetWorld()->GetTimerManager().ClearTimer(BoxSpawnTimer);
		return;
	}

	static const float SURVIVAL_BOX_MINIMUM_SPAWN_TIME = 0.5f;
	static const float SURVIVAL_BOX_SPAWN_DECREMENT = 0.02f;
	AdjustedSpawnRate = FMath::Max(SURVIVAL_BOX_MINIMUM_SPAWN_TIME, AdjustedSpawnRate - SURVIVAL_BOX_SPAWN_DECREMENT);

	if (BoxClasses.Num() > 0)
	{
		gameState->GetGrid()->SpawnRandomBoxInEachRegion(BoxClasses[FMath::RandRange(0, BoxClasses.Num() - 1)]);

		// consider moving some of this to gamestate where grid is now, or use delegates instead of all this grossness

		// TODO: also make it spawn faster at the start for like 5 boxes, maybe? just to fill grid? (rather than have dangling)
		// or just spawn a few skipping animation on beginplay - yea this is better

		// TODO: use an array to keep track of spawns per region.. so each region gets the same pattern (types of blocks, not locations)

		GetWorld()->GetTimerManager().SetTimer(BoxSpawnTimer, this, &AKBGameModeSurvival::SpawnBoxSurvival, AdjustedSpawnRate, false);
	}
}