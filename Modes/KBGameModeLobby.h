// Copyright Puny Human LLC 2020

#pragma once

#include "CoreMinimal.h"
#include "KickboxersGameMode.h"
#include "KBGameModeLobby.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API AKBGameModeLobby : public AKickboxersGameMode
{
	GENERATED_BODY()

public:

	AKBGameModeLobby();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController * newPlayer) override;
	
};
