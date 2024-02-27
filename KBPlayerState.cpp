// Fill out your copyright notice in the Description page of Project Settings.


#include "KBPlayerState.h"
#include "KBGameState.h"
#include "KickboxersPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Kickboxers.h"

void AKBPlayerState::BeginPlay()
{
	//SetIsOnlyASpectator(true);
	Super::BeginPlay();

	//PlayerReady = true;
}

void AKBPlayerState::SetPlayerReady_Server_Implementation(bool val)
{
	AKBGameState * gameState = Cast<AKBGameState>(GetWorld()->GetGameState());
	if (gameState && gameState->GameStarted == false && gameState->AllowingNewReadyPlayers())
	{
		if (PlayerReady != val)
		{
			SetIsOnlyASpectator(!val);
			PlayerReady = val;
			OnPlayerReadyStateChange.Broadcast(this, val);
		}
	}
}

void AKBPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKBPlayerState, PlayerReady);
	DOREPLIFETIME(AKBPlayerState, BoxesMatched);
	DOREPLIFETIME(AKBPlayerState, IsWinner);
	DOREPLIFETIME(AKBPlayerState, TeamNum);
}

void AKBPlayerState::OnBoxMatched()
{
	BoxesMatched++;

	if (HasAuthority())
	{
		UpdatePlayerBoxesMatched();

	}
}

void AKBPlayerState::OnRep_BoxesMatched()
{
	UpdatePlayerBoxesMatched();
}

void AKBPlayerState::UpdatePlayerBoxesMatched()
{
	AKickboxersPlayerController* controller = Cast<AKickboxersPlayerController>(GetOwner());
	if (IsValid(controller) && controller->IsLocalController())
	{
		controller->UpdateBoxesMatched(BoxesMatched);
	}
}

bool AKBPlayerState::IsPlayerAlive() const
{
	return (IsValid(GetPawn()) && GetPawn()->GetController() != nullptr && GetPawn()->IsActorInitialized());
}

void AKBPlayerState::PlayerMsg_Implementation(const FString& msg)
{
	KMSG(msg);
}