// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KickboxersGameMode.h"
#include "KBGameModePuzzle.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API AKBGameModePuzzle : public AKickboxersGameMode
{
	GENERATED_BODY()

public:
	virtual void StartBoxTimer() override;
	virtual void StartMatch() override;

protected:
	UFUNCTION()
	void OnBoxesMatched(const TArray<AKBBox*>& boxes, AKickboxersCharacter * const kicker);
};
