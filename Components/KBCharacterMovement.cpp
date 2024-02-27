// Fill out your copyright notice in the Description page of Project Settings.


#include "KBCharacterMovement.h"
#include "Actors/KBBox.h"
#include "Kickboxers.h"
#include "GameFramework/Character.h"

UKBCharacterMovement::UKBCharacterMovement()
{
	//MaxDepenetrationWithGeometry = 20.f;
	//MaxDepenetrationWithGeometryAsProxy = 10.f;
	//MaxDepenetrationWithPawn = 10.f;
	//MaxDepenetrationWithPawnAsProxy = 2.f;

	PerchRadiusThreshold = 40.f;
	AirControl = 0.f; // Prevent the player from air controlling themselves off of ledges if they become airborne due to LaunchCharacter

	// Instantly rotate to whatever our direction is
	bUseControllerDesiredRotation = false;
	bOrientRotationToMovement = false;
	RotationRate = FRotator(0.f, 50000.f, 0.f);
}

FVector UKBCharacterMovement::GetPenetrationAdjustment(const FHitResult& Hit) const
{
	return Super::GetPenetrationAdjustment(Hit);// FVector::ZeroVector;
}


//bool UKBCharacterMovement::CheckLedgeDirection(const FVector& OldLocation, const FVector& SideStep, const FVector& GravDir) const
//{
//	// Same as UCharacterMovementComponent::CheckLedgeDirection but with a different collision shape to prevent hanging off the side of ledges
//	const FVector SideDest = OldLocation + SideStep;
//	FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(CheckLedgeDirection), false, CharacterOwner);
//	FCollisionResponseParams ResponseParam;
//	InitCollisionParams(CapsuleParams, ResponseParam);
//	const FCollisionShape CapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_RadiusCustom, 30.f);
//	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
//	FHitResult Result(1.f);
//	GetWorld()->SweepSingleByChannel(Result, OldLocation, SideDest, FQuat::Identity, CollisionChannel, CapsuleShape, CapsuleParams, ResponseParam);
//
//	if (!Result.bBlockingHit || IsWalkable(Result))
//	{
//		if (!Result.bBlockingHit)
//		{
//			GetWorld()->SweepSingleByChannel(Result, SideDest, SideDest + GravDir * (MaxStepHeight + LedgeCheckThreshold), FQuat::Identity, CollisionChannel, CapsuleShape, CapsuleParams, ResponseParam);
//		}
//		if ((Result.Time < 1.f) && IsWalkable(Result))
//		{
//			return true;
//		}
//	}
//	return false;
//}

//void UKBCharacterMovement::ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult) const
//{
//	// Override with a smaller radius so there's less floating off of edges
//	Super::ComputeFloorDist(CapsuleLocation, LineDistance, SweepDistance, OutFloorResult, 5.f, DownwardSweepResult);
//}