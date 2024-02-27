// Copyright Puny Human LLC 2020


#include "KBGridCellComponent.h"
#include "KBDefinitions.h"
#include "Actors/KBGrid.h"
#include "KBGameState.h"

// Sets default values for this component's properties
UKBGridCellComponent::UKBGridCellComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...

}


// Called when the game starts
void UKBGridCellComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AKBGameState * const state = GetWorld()->GetGameState<AKBGameState>())
	{
		if (AKBGrid * grid = state->GetGrid())
		{
			Grid = grid;
			FIntPoint gridCell = grid->GetCellFromWorldLocation(GetOwner()->GetActorLocation());
			if (grid->IsPointWithinGrid(gridCell))
			{
				SetGridCell(gridCell);
				grid->AddGridComponent(this);

				if (SnapOwnerToGridLocationOnBeginPlay)
				{
					GetOwner()->SetActorLocation(grid->GetCellLocation(gridCell));
				}
			}
			else
			{
				WARNING("UKBGridCellComponent::BeginPlay - Grid actor spawned outside the boundaries of the grid");
			}
		}
		else
		{
			WARNING("UKBGridCellComponent::BeginPlay - Gamestate has no valid grid, aborting");
		}
	}
	else
	{
		WARNING("UKBGridCellComponent::BeginPlay - Not using AKBGameState, aborting grid component setup");
	}
}

void UKBGridCellComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (Grid.IsValid())
	{
		Grid->RemoveGridComponent(this);
	}
}

// Called every frame
void UKBGridCellComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

bool UKBGridCellComponent::IsCellClear() const
{
	// First check if this component's actor is blocking
	if (IsActorBlocking())
	{
		return false;
	}
	else if (Grid.IsValid())
	{
		return Grid->IsCellClear(Cell);
	}

	// If we're not on a grid currently we have no location data and thus no way of knowing if the cell is blocked
	return false;
}

bool UKBGridCellComponent::IsActorBlocking() const
{
	TInlineComponentArray<USceneComponent*> components(GetOwner());
	for (const auto& component : components)
	{
		if (component && component->IsCollisionEnabled() &&
			component->GetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn) == ECollisionResponse::ECR_Block)
		{
			return true;
		}
	}

	return false;
}