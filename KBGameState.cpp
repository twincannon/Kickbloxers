// Fill out your copyright notice in the Description page of Project Settings.


#include "KBGameState.h"
#include "KBPlayerState.h"
#include "KickBoxersGameMode.h"
#include "KickboxersPlayerController.h"
#include "Actors/KBGrid.h"
#include "Net/UnrealNetwork.h"

//void AKBGameState::StartCountDown()
//{
//	//PlayerArray[0].GetPawn();
//	//GetMatchState();
//	
//	
//	// ok change this to like.. ChangeMatchState and do COUNTING_DOWN ... and then if counting down, fire off a delegate.
//	// or make this bind to that in the gamemode etc.
//	// repnotify shouold be what drives this in any case
//	// I dunno if i want another state var, maybe just "OnAllPlayersReady" and "OnCountdownAborted"
//
//	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
//	{
//		APlayerController* const PlayerController = Iterator->Get();
//		if (PlayerController)
//		{
//			
//		}
//	}
//
//}

// post initialize components -> get game mode and bind our events

// actually this is dumb just have game mode talk to playercontrollers directly

void AKBGameState::BeginPlay()
{
	Super::BeginPlay();

	GameStarted = false;
}

void AKBGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AKBGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKBGameState, Grid);
	DOREPLIFETIME(AKBGameState, GameSettings);
}

void AKBGameState::OnLobbyLoaded_Multicast_Implementation()
{
	OnLobbyLoaded.Broadcast();
}

bool AKBGameState::SetGrid(AKBGrid * const newGrid)
{
	if (!ensureMsgf(Grid == nullptr, TEXT("AKBGameState::SetGrid - Already have a valid grid, aborting")))
	{
		return false;
	}

	if (newGrid)
	{
		Grid = newGrid;
		return true;
	}

	return false;
}

void AKBGameState::AddPlayerState(APlayerState* playerState)
{
	Super::AddPlayerState(playerState);

	AKBPlayerState * kbState = Cast<AKBPlayerState>(playerState);
	if (kbState)
	{
		kbState->OnPlayerReadyStateChange.AddDynamic(this, &AKBGameState::OnReadyStateChanged);
	}
}

void AKBGameState::OnReadyStateChanged(AKBPlayerState * const playerState, bool isReady)
{
	if (isReady && !AllowingNewReadyPlayers() && !GameStarted)
	{
		return;
	}

	int32 oldReadyPlayers = ReadyPlayers.Num();

	if (isReady)
	{
		ReadyPlayers.AddUnique(playerState);
	}
	else
	{
		ReadyPlayers.Remove(playerState);
	}

	if (HasAuthority() && Grid)
	{
		if (ReadyPlayers.Num() >= Grid->NumPlayersRequired + Grid->NumPlayersOptional)
		{
			OnAllPlayersReady.Broadcast();
		}
		else if (ReadyPlayers.Num() >= Grid->NumPlayersRequired && oldReadyPlayers < Grid->NumPlayersRequired)
		{
			OnRequiredPlayersReady.Broadcast();
		}
	}

	const int32 numReady = GetNumReadyPlayers();
	OnNumPlayersReadyChanged.Broadcast(PlayerArray.Num(), numReady, Grid->NumPlayersRequired, Grid->NumPlayersOptional);
}

bool AKBGameState::AllowingNewReadyPlayers()
{
	if (!Grid) { return false; } // I hate this, try and move the vars into this state where they belong?

	// Check if ready list is already completely full
	return (ReadyPlayers.Num() < Grid->NumPlayersRequired + Grid->NumPlayersOptional);
}

int32 AKBGameState::GetNumReadyPlayers() const
{
	int32 numReady = 0;
	for (const auto& playerState : PlayerArray)
	{
		AKBPlayerState * kbState = Cast<AKBPlayerState>(playerState);
		if (kbState && kbState->IsPlayerReady())
		{
			numReady++;
		}
	}
	return numReady;
}

void AKBGameState::OnMatchStarted_Multicast_Implementation()
{
	OnMatchStarted.Broadcast();
}

int32 AKBGameState::GetReadyPlayerIndex(AKBPlayerState * playerState)
{
	return ReadyPlayers.Find(playerState);
}

void AKBGameState::OnRoundComplete()
{
	// Round/match complete, update player HUDs
	for (const auto& state : PlayerArray)
	{
		AKBPlayerState * kbps = Cast<AKBPlayerState>(state);
		if (kbps)
		{
			AKickboxersPlayerController* controller = Cast<AKickboxersPlayerController>(kbps->GetOwner());
			if (IsValid(controller))
			{
				// Let all players know round completed
				controller->OnRoundComplete_Client();

				if (kbps->IsOnlyASpectator() == false)
				{
					if (kbps->GetIsWinner())
					{
						controller->OnPlayerWon_Client();
					}
					else
					{
						controller->OnPlayerLost_Client();
					}
				}
			}
		}
	}
}

void AKBGameState::OnPlayerDied()
{
	// TODO: this should be driven by game mode because it doesn't make sense in some modes

	// Check if game is over
	TArray<AKBPlayerState*> alivePlayers;
	alivePlayers.Reserve(PlayerArray.Num());

	for (const auto& state : PlayerArray)
	{
		AKBPlayerState * kbps = Cast<AKBPlayerState>(state);
		if (kbps)
		{
			if (kbps->IsPlayerAlive())
			{
				alivePlayers.Emplace(kbps);
			}
		}
	}

	if (alivePlayers.Num() == 0)
	{
		OnNoPlayersAlive.Broadcast();
		return;
	}
	else if (alivePlayers.Num() == 1)
	{
		OnLastPlayerAlive.Broadcast(alivePlayers[0]);
		return;
	}

	int32 teamNum = INDEX_NONE;
	for (const auto& state : alivePlayers)
	{
		if (teamNum != INDEX_NONE && teamNum != state->GetTeamNum())
		{
			// More than one team is still alive
			return;
		}
		else
		{
			teamNum = state->GetTeamNum();
		}
	}

	if (teamNum != INDEX_NONE)
	{
		OnLastTeamAlive.Broadcast(alivePlayers);
	}
}

void AKBGameState::SendPlayerMsg(const FString& msg)
{
	for (const auto& playerState : PlayerArray)
	{
		AKBPlayerState * kbState = Cast<AKBPlayerState>(playerState);
		if (kbState)
		{
			kbState->PlayerMsg(msg);
		}
	}
}