// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "KBPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerReadyStateChange, AKBPlayerState * const, playerState, bool, isReady);

/**
 * 
 */
UCLASS()
class KICKBOXERS_API AKBPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool IsPlayerReady() const { return PlayerReady; }

	UFUNCTION(Reliable, Server)
	void SetPlayerReady_Server(bool val);

	FOnPlayerReadyStateChange OnPlayerReadyStateChange;

	void OnBoxMatched();

	void UpdatePlayerBoxesMatched();

	bool GetIsWinner() const { return IsWinner; }
	void SetIsWinner(bool val) { IsWinner = val; }

	bool IsPlayerAlive() const;

	int32 GetTeamNum() const { return TeamNum; }
	void SetTeamNum(int32 val) { TeamNum = val; }

	UFUNCTION(Reliable, Client)
	void PlayerMsg(const FString& msg);

protected:
	UPROPERTY(Replicated)
	bool IsWinner = false;

	UPROPERTY(Replicated)
	bool PlayerReady = false;

	UPROPERTY(Replicated, ReplicatedUsing = "OnRep_BoxesMatched")
	int32 BoxesMatched = 0;

	UPROPERTY(Replicated)
	int32 TeamNum = INDEX_NONE;

	UFUNCTION()
	void OnRep_BoxesMatched();
};
