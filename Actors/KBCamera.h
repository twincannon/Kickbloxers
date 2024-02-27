// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "KBCamera.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API AKBCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	// Angle offset to use for movement (i.e. moving isometric means the player hits up and moves 45 degrees to north)
	UPROPERTY(Category = "Kickboxers", EditInstanceOnly)
	float AngleOffset = 0.f;

	// Force all players to use this camera (useful for single player modes etc.)
	UPROPERTY(Category = "Kickboxers", EditInstanceOnly)
	bool ForceUseCamera = false;

	virtual void BeginPlay() override;
};
