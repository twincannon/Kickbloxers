// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "KBDefinitions.h"
#include "KBGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyLoaded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRequiredPlayersReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllPlayersReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMatchStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnNumPlayersReadyChanged, int32, totalPlayers, int32, readyPlayers, int32, requiredPlayers, int32, optionalPlayers);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastPlayerAlive, AKBPlayerState * const, state);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastTeamAlive, TArray<AKBPlayerState*>, states);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNoPlayersAlive);

class AKBPlayerState;
class AKBGrid;

/**
 * 
 */
UCLASS()
class KICKBOXERS_API AKBGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void AddPlayerState(APlayerState* PlayerState) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(NetMulticast, Reliable)
	void OnLobbyLoaded_Multicast();

	int32 GetNumReadyPlayers() const;

	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnLobbyLoaded OnLobbyLoaded;

	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnRequiredPlayersReady OnRequiredPlayersReady;

	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnAllPlayersReady OnAllPlayersReady;

	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnMatchStarted OnMatchStarted;

	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnNumPlayersReadyChanged OnNumPlayersReadyChanged;

	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnLastPlayerAlive OnLastPlayerAlive;

	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnLastTeamAlive OnLastTeamAlive;

	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnNoPlayersAlive OnNoPlayersAlive;

	UFUNCTION(NetMulticast, Unreliable)
	void OnMatchStarted_Multicast();

	AKBGrid * GetGrid() const { return Grid; }

	// Returns if setting was successful
	bool SetGrid(AKBGrid * const newGrid);

	int32 GetReadyPlayerIndex(AKBPlayerState * playerState);

	bool AllowingNewReadyPlayers();

	void OnRoundComplete();

	void OnPlayerDied();

	UPROPERTY(Replicated)
	FGameSettings GameSettings;

	void SendPlayerMsg(const FString& msg);

	bool OverrideSproutGrowTimer = false;

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	bool HasGameStarted() const { return GameStarted; }

	bool GameStarted = false; // If the game has started (gamemode match state unrelated)

protected:

	// The grid actor to use (there should be exactly 1 grid actor in a gameplay level)
	UPROPERTY(Replicated)
	AKBGrid * Grid = nullptr;

	TArray<TWeakObjectPtr<AKBPlayerState>> ReadyPlayers;

	UFUNCTION()
	void OnReadyStateChanged(AKBPlayerState * const playerState, bool isReady);
	
};
