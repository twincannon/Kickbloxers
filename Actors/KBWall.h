// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KBDefinitions.h"
#include "Kickboxers.h"
#include "KBWall.generated.h"

class AKBGrid;
class UBoxComponent;

DECLARE_DELEGATE_TwoParams(FWallRequestMoveDelegate, AKBWall * const, ECardinalDirection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWallFinishedMovingDelegate, AKBWall * const, wall);

USTRUCT()
struct FWallQueuedMove 
{
	GENERATED_USTRUCT_BODY()
	AKBWall * Wall = nullptr; 
	ECardinalDirection Direction = ECardinalDirection::NORTH;
};

UCLASS()
class KICKBOXERS_API AKBWall : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AKBWall();

#if WITH_EDITOR
	// Prevent editor gizmo rotation
	virtual void EditorApplyRotation(const FRotator & DeltaRotation, bool bAltDown, bool bShiftDown, bool bCtrlDown	) override { };
	virtual void PostEditChangeProperty(FPropertyChangedEvent & propertyChangedEvent) override;
#endif

	FWallRequestMoveDelegate OnWallRequestMove;
	FWallFinishedMovingDelegate OnWallFinishedMoving;

	UFUNCTION(Category = "Kickboxers", BlueprintCallable)
	void WallFinishedMoving();

	
	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	UBoxComponent * WallCollision;

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	UBoxComponent * PlayerPush;

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	class UStaticMeshComponent * WallMesh;

	// The priority of the wall in region creation (higher priority is parsed earlier)
	UPROPERTY(Category = "Kickboxers", BlueprintReadOnly, EditAnywhere)
	int32 WallPriority = 0;

	// This sets the box collision extents such that we can just use increments of 1 to scale the wall to # of cells wide/high
	void SetWallBaseScale(FVector2D cellSize);

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	bool IsHorizontal() const;

	void AddPush(ECardinalDirection pushDir, FVector sourceOfPush);
	
	UFUNCTION(Category = "Kickboxers", BlueprintCallable)
	void TryApplyPush();

	UFUNCTION(Client, Unreliable)
	void SpawnPushParticle_Client(ECardinalDirection pushDir, FVector sourceOfPush);

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void SpawnPushParticle(ECardinalDirection pushDir, FVector sourceOfPush);

	void SetNorthwestCell(FIntPoint cell) { NorthwestCell = cell; };

	void UpdateWallLocAndScale(const FIntPoint& nearestCell, const FVector2D& cellSize, const FVector& nearestCellLoc, const FVector& regionStartLoc, const FGridRegion* const region, bool applyImmediately = false);


	const FVector& GetWallTargetLocation() const { return TargetLocation; }
	void SetWallTargetLocation(const FVector& newLoc) { TargetLocation = newLoc; }

	TArray<AActor*> GetNeighboringActors(ECardinalDirection direction, const AKBGrid * const grid);

	bool IsWallMoving() const { return WallMoving; }
	bool IsWallAtEdge() const { return WallAtEdge; }
	void SetWallAtEdge(bool val) { WallAtEdge = val; }

	// If this wall should split the entire grid horizontally or vertically (and not just the region it's in)
	UPROPERTY(Category = "Kickboxers", EditAnywhere)
	bool SplitEntireGrid = false;

protected:
	UPROPERTY(Category = "Kickboxers", BlueprintReadOnly)
	int32 StoredPush;

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly)
	float PlayerPushPadding = 10.f;

	bool WallAtEdge = false;

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite)
	bool WallMoving = false;

	void UpdatePlayerPushScale();

	UFUNCTION(NetMulticast, Unreliable)
	void OnWallPushApplied_Multicast();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnWallPushApplied();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnWallMoved(ECardinalDirection direction);

	// Try and push players away. Multicast to try and smooth out client view
	UFUNCTION(Category = "Kickboxers", BlueprintCallable, NetMulticast, Unreliable)
	void ApplyPlayerPush();

	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void MoveWallTo_Internal(FVector newLocation);

	UFUNCTION(Reliable, Client)
	void UpdateWallLocAndScale_Client();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void MoveWallTo(const FVector& newLocation, const FVector& newScale);

	// Target location and scale that gets replicated to client
	UPROPERTY(Replicated)
	FVector_NetQuantize100 TargetLocation = FVector::ZeroVector;
	
	UPROPERTY(Replicated)
	FVector_NetQuantize100 TargetScale = FVector::OneVector;

	UPROPERTY(Replicated)
	FVector_NetQuantize100 CurrentLocation = FVector::ZeroVector;

	UPROPERTY(Replicated)
	FVector_NetQuantize100 CurrentScale = FVector::OneVector;

private:
	FIntPoint NorthwestCell = FIntPoint::NoneValue;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
