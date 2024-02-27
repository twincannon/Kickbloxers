// Copyright Puny Human LLC 2020


#include "KBGameModeLobby.h"
#include "KBGameState.h"
#include "KickboxersPlayerController.h"

AKBGameModeLobby::AKBGameModeLobby()
{
	bDelayedStart = false;
	MatchReady = true;
}

void AKBGameModeLobby::BeginPlay()
{
	Super::BeginPlay();

	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		gameState->OnLobbyLoaded_Multicast();
	}
}

void AKBGameModeLobby::PostLogin(APlayerController * newPlayer)
{
	Super::PostLogin(newPlayer);

	if (AKickboxersPlayerController * kbPlayer = Cast<AKickboxersPlayerController>(newPlayer))
	{
		kbPlayer->OnLoginToLobby();
	}
}
