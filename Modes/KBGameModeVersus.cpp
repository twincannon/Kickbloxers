// Fill out your copyright notice in the Description page of Project Settings.


#include "KBGameModeVersus.h"
#include "KBGameState.h"
#include "KBPlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/PlayerStartPIE.h"
#include "Actors/KBPlayerStart.h"
#include "EngineUtils.h"
#include "Actors/KBCamera.h"

AActor* AKBGameModeVersus::FindPlayerStart_Implementation(AController* player, const FString& incomingName)
{
	UWorld* World = GetWorld();

	AKBGameState * gameState = GetGameState<AKBGameState>();
	check(gameState);

	int32 playerReadyIndex = INDEX_NONE;
	AKBPlayerState * playerState = player->GetPlayerState<AKBPlayerState>();
	if (playerState)
	{
		playerReadyIndex = gameState->GetReadyPlayerIndex(playerState);
	}

	AActor * outStart = nullptr;

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		// Check for PIE "play from here" start
		APlayerStart* playerStart = *It;
		if (playerStart->IsA<APlayerStartPIE>())
		{
			outStart = playerStart;
			break;
		}

		// Check for our custom player start
		if (AKBPlayerStart * kbStart = Cast<AKBPlayerStart>(*It))
		{
			if (kbStart->StartIndex == playerReadyIndex)
			{
				playerState->SetTeamNum(kbStart->TeamNum);
				outStart = kbStart;
				break;
			}
		}

		// Fall back to regular player starts using their tag as an index
		if (playerStart->PlayerStartTag.IsEqual(FName(*FString::FromInt(playerReadyIndex))))
		{
			outStart = playerStart;
			break;
		}
	}

	// Fallback to default spawn finding
	if (outStart == nullptr)
	{
		outStart = Super::FindPlayerStart_Implementation(player, incomingName);
	}

	return outStart;
}

void AKBGameModeVersus::StartMatch()
{
	Super::StartMatch();

	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		// Versus game mode is about eliminating other players, so check via these
		gameState->OnLastPlayerAlive.AddDynamic(this, &AKBGameModeVersus::LastPlayerAlive);
		gameState->OnLastTeamAlive.AddDynamic(this, &AKBGameModeVersus::LastTeamAlive);
	}

}

void AKBGameModeVersus::LastPlayerAlive(AKBPlayerState * const playerState)
{
	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		// Only one player left so he must be the winner
		playerState->SetIsWinner(true);
		EndMatch();
	}
}

void AKBGameModeVersus::LastTeamAlive(TArray<AKBPlayerState*> playerStates)
{
	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		// One team left, they're all winners
		for (const auto& state : playerStates)
		{
			state->SetIsWinner(true);
		}

		EndMatch();
	}
}