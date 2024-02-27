// Copyright Puny Human LLC 2020

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KBGridCellComponent.generated.h"

class AKBGrid;
class AKickboxersCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerLeftCell);

//test
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KICKBOXERS_API UKBGridCellComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UKBGridCellComponent();

	// Called when the player moves out of the cell this gc is representing
	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnPlayerLeftCell OnPlayerLeftCell;

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	FIntPoint GetGridCell() const { return Cell; }

	void SetGridCell(const FIntPoint& newCell) { Cell = newCell; }

	// If the cell is blocked (block response on pawn collision channel)
	bool IsCellClear() const;

	//need unique actor type for cells i.e. cant have 2 sprouts (maybe not? just query grid?)

	// If this component's specific actor is blocking the pawn channel
	bool IsActorBlocking() const;

	void ResetGridCell() { Cell = FIntPoint::NoneValue; }
	bool HasValidCell() const { return Cell != FIntPoint::NoneValue; }

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	AKBGrid * GetGrid() const { return Grid.Get(); }

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly)
	bool SnapOwnerToGridLocationOnBeginPlay = true;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	FIntPoint Cell = FIntPoint::NoneValue;
	TWeakObjectPtr<AKBGrid> Grid;

	// on grid entered
	// on grid exited (this would be in grid actor)

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
