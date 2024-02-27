// Fill out your copyright notice in the Description page of Project Settings.


#include "KBGameModePuzzle.h"
#include "Actors/KBGrid.h"
#include "Actors/KBBox.h"
#include "KBGameState.h"
#include "KBPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "KickboxersCharacter.h"

// starts when exactly one player is ready (and maybe auto-ready them)
// no box spawning, boxes are pre-spawned in level
// ends when no boxes are left (onboxmatched delegate)
// stretch goal: undo?

void AKBGameModePuzzle::StartBoxTimer()
{
	// Don't start the timer in puzzle mode
}

void AKBGameModePuzzle::StartMatch()
{
	Super::StartMatch();

	AKBGameState * gameState = GetGameState<AKBGameState>();
	if (gameState && gameState->GetGrid())
	{
		gameState->GetGrid()->OnBoxesMatchedDelegate.AddDynamic(this, &AKBGameModePuzzle::OnBoxesMatched);
		GetWorld()->GetTimerManager().ClearTimer(BoxSpawnTimer);
		return;
	}
}

void AKBGameModePuzzle::OnBoxesMatched(const TArray<AKBBox*>& boxes, AKickboxersCharacter * const kicker)
{
	// get all boxes and see if any are valid for matches - if none, player wins
	// also check if >= num required for match exist (and move this var into the box class)


	TArray<AActor*> boxActors;
	UGameplayStatics::GetAllActorsOfClass(this, AKBBox::StaticClass(), boxActors);

	bool anyBoxValid = false;
	for (const auto& actor : boxActors)
	{
		if (AKBBox * box = Cast<AKBBox>(actor))
		{
			if (box->IsValidForMatches())
			{
				anyBoxValid = true;
				break;
			}
		}
	}

	if (!anyBoxValid)
	{
		AKBPlayerState * playerState = kicker->GetPlayerState<AKBPlayerState>();
		if (playerState)
		{
			playerState->SetIsWinner(true);
		}

		EndMatch();
	}
	
	// TODO: Check and see if any matches are still possible (go through each, GetClass(), and store em)


}