// Copyright Puny Human LLC 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "KBSpectatorPawn.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API AKBSpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* newController) override;
};
