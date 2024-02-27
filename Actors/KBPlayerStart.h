// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "KBPlayerStart.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API AKBPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	// Start index, the order that players ready up determine which spawn index they use
	UPROPERTY(Category = "Kickboxers", EditInstanceOnly)
	int32 StartIndex = 0;

	// Team number (-1 is no team/FFA)
	UPROPERTY(Category = "Kickboxers", EditInstanceOnly)
	int32 TeamNum = INDEX_NONE;

	// Camera to use
	UPROPERTY(Category = "Kickboxers", EditInstanceOnly)
	ACameraActor * CameraToUse;
	
};
