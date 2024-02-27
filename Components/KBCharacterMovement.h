// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KBCharacterMovement.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API UKBCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UKBCharacterMovement();

	/** Overridden to enforce max distances based on hit geometry. */
	virtual FVector GetPenetrationAdjustment(const FHitResult& Hit) const override;

	//virtual bool ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation) override;

//	virtual bool CheckLedgeDirection(const FVector& OldLocation, const FVector& SideStep, const FVector& GravDir) const override;

	//virtual void ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult = NULL) const override;
};
