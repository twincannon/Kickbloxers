// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KickboxersGameMode.h"
#include "KBGameModeVersus.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API AKBGameModeVersus : public AKickboxersGameMode
{
	GENERATED_BODY()

public:
	// Custom FindPlayerStart to make sure players start in their appropriate regions
	virtual AActor * FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

	virtual void StartMatch() override;

protected:
	UFUNCTION()
	void LastPlayerAlive(AKBPlayerState * const playerState);
	UFUNCTION()
	void LastTeamAlive(TArray<AKBPlayerState*> playerStates);
};
