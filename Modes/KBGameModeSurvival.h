// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KickboxersGameMode.h"
#include "KBGameModeSurvival.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API AKBGameModeSurvival : public AKickboxersGameMode
{
	GENERATED_BODY()

public:
	virtual void StartBoxTimer() override;

	//virtual void SpawnBox_Implementation() override;

	inline virtual int32 GetMinimumEmptyCells() const override { return 0; }

private:
	float AdjustedSpawnRate;

	void SpawnBoxSurvival();
};
