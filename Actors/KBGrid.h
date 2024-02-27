// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KBDefinitions.h"
#include "KBWall.h"
#include "KBGrid.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBoxesMatched, const TArray<AKBBox*>&, boxes, AKickboxersCharacter * const, kicker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerTriedMovingOutsideGrid, AKickboxersCharacter *, player, FIntPoint, targetPoint);
class AKBBox;
class AKBSprout;
class AKBWall;
class UBoxComponent;
class AKickboxersCharacter;
class UKBGridCellComponent;

// Not using this for now, might be useful in future for grid cell types though (not tracking the objects within the grid)

//USTRUCT()
//struct FGridCell
//{
//	GENERATED_BODY()
//
//public:
//	TArray<TWeakObjectPtr<UKBGridCellComponent>> Comps;
//
//	// Maybe other data about the cell (open air cells - ground type, etc)
//};
//
//USTRUCT()
//struct FGridCellArray
//{
//	GENERATED_BODY()
//
//public:
//	TArray<FGridCell> Cells;
//
//	FGridCell operator[] (int32 i)
//	{
//		return Cells[i];
//	}
//
//	void Add(const FGridCell& cell)
//	{
//		Cells.Add(cell);
//	}
//
//	int32 Num() const { return Cells.Num(); }
//	bool IsValidIndex(int32 index) const { return index >= 0 && index < Cells.Num(); }
//};

UCLASS()
class KICKBOXERS_API AKBGrid : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AKBGrid();

	//TArray<FGridCellArray> Cells;
	//inline bool IsCellsPointValid(FIntPoint point) const
	//{
	//	return Cells.IsValidIndex(point.X) && Cells[point.X].IsValidIndex(point.Y);
	//}

	void AddGridComponent(UKBGridCellComponent * cellComp);
	void RemoveGridComponent(UKBGridCellComponent * cellComp);

	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnBoxesMatched OnBoxesMatchedDelegate;

	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
		FOnPlayerTriedMovingOutsideGrid OnPlayerTriedMovingOutsideGrid;

	// Number of players required to start the game using this grid
	UPROPERTY(Category = "Kickboxers", EditAnywhere)
	int32 NumPlayersRequired = 1;

	// Number of players that can optionally also join beyond the required amount
	UPROPERTY(Category = "Kickboxers", EditAnywhere)
	int32 NumPlayersOptional = 0;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent & propertyChangedEvent) override;
	virtual void PostEditMove(bool finished) override;
#endif
	
	//UFUNCTION(Reliable, NetMulticast) // We want clients to get this any time after they connect
	void InitializeGrid(bool firstTime = false); // todo: we only need the wall resizing to be replicated

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Tick(float DeltaTime) override;

	FVector GetCellLocation(int32 x, int32 y) const;
	FVector GetCellLocation(FIntPoint point) const;

	FIntPoint GetGridSize() const { return GridSize; }
	FVector2D GetCellSize() const { return CellSize; }

	float GetCellDimension(bool horizontal) const { return horizontal ? CellSize.X : CellSize.Y; }

	void CreateGrid();

	void SpawnSproutInEachRegion(TSubclassOf<AKBSprout> sproutClass);

	void SpawnRandomBoxInEachRegion(TSubclassOf<AKBBox> boxClass);

	UFUNCTION(Category = "Kickboxers", BlueprintCallable)
	AKBBox * SpawnBoxInCell(FIntPoint cell, TSubclassOf<AKBBox> boxClass, EBoxMoveType spawnMoveType, AKickboxersCharacter * const optionalKicker = nullptr);

	void SpawnRandomSproutInCell(FIntPoint cell);

	bool OnBoxPlaced(AKBBox * placedBox);
	
	void FindMatches(AKBBox * const currentBox, TArray<AKBBox*>& boxesToCheck, TArray<AKBBox*>& outBoxes) const;
	AKBBox * FindBoxInNeighboringCell(AKBBox* const sourceBox, const FIntPoint& offset, const TArray<AKBBox*>& boxesToCheck) const;

	// Gets nearest cell from world location (floors X/Y so always returns north-most, west-most cell)
	// If floor is false, value will be rounded instead 
	FIntPoint GetCellFromWorldLocation(FVector loc, bool clampToGridBounds = true, bool floor = true) const;

	FGridRegion* GetCellRegion(const FIntPoint& cell) const;

	bool IsSameRegion(const FIntPoint& cellA, const FIntPoint& cellB) const;

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	AKBBox * GetBoxInCell(const FIntPoint& cell) const;

	AKBSprout * GetSproutInCell(const FIntPoint& cell) const;

	const TArray<FGridRegion>& GetGridRegions() const { return GridRegions; }

	TArray<AKBBox*> GetBoxesInRegion(const FGridRegion* const region) const;
	TArray<AKBBox*> GetBoxesInRegion(const FIntPoint& sourceCell) const; // Overload for getting boxes in target cell's region

	bool IsPointWithinGrid(FIntPoint point) const;

	// Checks if location is within grid (only checks x/y)
	bool IsLocationWithinGrid(const FVector& location, const FVector& padding = FVector::ZeroVector) const;

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	bool IsCellClear(FIntPoint cell) const;
	bool IsCellClear(FIntPoint cell, TArray<AActor*> actorsToIgnore) const;
	bool IsCellClear(FIntPoint cell, TArray<AActor*> actorsToIgnore, AActor *& outBlockingActor) const;

	UFUNCTION(Unreliable, Server)
	void CheckStuckCharacters_Server();

	void NotifyGridComponentsOfPlayerMove(FIntPoint playerPrevCell);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

#if !SHIP_OR_TEST
	void Debug_CreateGridText();
#endif

	TArray<TWeakObjectPtr<UKBGridCellComponent>> CellComps;

	UFUNCTION()
	virtual void PostLoad() override;

	UFUNCTION(Reliable, NetMulticast)
	void ApplyWallMoveToBoxes_Multicast(const TArray<AKBBox*>& boxes, const TArray<FVector>& locations);

	UPROPERTY(Category = "Kickboxers", BlueprintReadOnly, EditAnywhere)
	FIntPoint GridSize = FIntPoint(32, 18);

	UPROPERTY(Category = "Kickboxers", BlueprintReadOnly, EditAnywhere)
	FVector2D CellSize = FVector2D(100.f, 100.f);

	UPROPERTY(Category = "Kickboxers", BlueprintReadOnly, EditDefaultsOnly)
	USceneComponent * GridRoot;

	UPROPERTY(Category = "Kickboxers", VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent * GridBoxComp;

	// TODO: this actor still isnt replicated, so this doesnt work.. figure that out
	UPROPERTY(/*Replicated)//,*/ ReplicatedUsing = "OnRep_GridRegions")
	TArray<FGridRegion> GridRegions;
	UFUNCTION()
	void OnRep_GridRegions();

	// Using array as we will be modifying the queue sometimes
	TArray<FWallQueuedMove> WallMoveQueue;

	UFUNCTION()
	void OnWallRequestedMove(AKBWall * const movingWall, ECardinalDirection dir);

	void DoNextWallMove();

	UFUNCTION()
	void OnWallFinishedMove(AKBWall * const wall);

	//UFUNCTION()
	//void OnRep_AnimState();

	// Splits a region based on given point, modifies inRegion with new dimensions and overwrites outRegion with all accurate data
	FGridRegion* SplitRegion(FGridRegion* inRegion, const FIntPoint& splitPoint, AKBWall * const splittingWall);

};
