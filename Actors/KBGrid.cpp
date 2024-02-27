// Fill out your copyright notice in the Description page of Project Settings.



#include "KBGrid.h"
#include "Kickboxers.h"
#include "KickBoxers/KickboxersGameMode.h"
#include "KBBox.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/TextRenderComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/KBGridCellComponent.h"
#include "Kismet/GameplayStatics.h"
#include "KickboxersCharacter.h"
#include "Net/UnrealNetwork.h"
#include "KBUtility.h"
#include "KBGameState.h"
#include "KBPlayerState.h"
#include "Actors/KBSprout.h"
#include "EngineUtils.h"

// Sets default values
AKBGrid::AKBGrid()
{
	GridRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GridRoot"));
	GridRoot->SetHiddenInGame(true);
	RootComponent = GridRoot;

	GridBoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("ShipCollision"));
	GridBoxComp->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	GridBoxComp->SetCollisionObjectType(COLLISION_GRID);
	GridBoxComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	GridBoxComp->SetCollisionResponseToChannel(COLLISION_BOX, ECollisionResponse::ECR_Overlap);
	GridBoxComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap); // For AKBWall actors
	//GridBoxComp->SetLineThickness(5.f); // TODO: if this project ever gets a dedicated engine repro, implement this function in UBoxComponent
	GridBoxComp->ShapeColor = FColor::Magenta;
	GridBoxComp->SetupAttachment(RootComponent);


 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	//bOnlyRelevantToOwner = false;
	//bAlwaysRelevant = true;
	
	//bNetLoadOnClient = false; //this actually breaks stuff, hmm
}

void AKBGrid::PostLoad()
{
	Super::PostLoad();
#if !SHIP_OR_TEST
	if (!IsTemplate() && !HasAnyFlags(RF_ClassDefaultObject))
	{
		CreateGrid();
	}
#endif
}

void AKBGrid::PostInitializeComponents()
{
	CreateGrid();

	if (!IsTemplate() && !HasAnyFlags(RF_ClassDefaultObject))
	{
		AKBGameState * const state = GetWorld()->GetGameState<AKBGameState>();
		if (HasAuthority() && state)
		{
			state->SetGrid(this);
		}
		
		InitializeGrid(true);
	}

	Super::PostInitializeComponents();
}

// Called when the game starts or when spawned
void AKBGrid::BeginPlay()
{
	Super::BeginPlay();	
}

void AKBGrid::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//DOREPLIFETIME(AKBGrid, GridRegions);
	DOREPLIFETIME_CONDITION_NOTIFY(AKBGrid, GridRegions, COND_None, REPNOTIFY_Always);
}

void AKBGrid::OnRep_GridRegions()
{
	
}

// Called every frame
void AKBGrid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


#if WITH_EDITOR
void AKBGrid::PostEditChangeProperty(FPropertyChangedEvent & propertyChangedEvent)
{
	Super::PostEditChangeProperty(propertyChangedEvent);

	if (!IsTemplate())
	{
		CreateGrid();
	}
}

void AKBGrid::PostEditMove(bool finished)
{
	Super::PostEditMove(finished);

	if (!IsTemplate())
	{
		CreateGrid();
	}
}
#endif

void AKBGrid::CreateGrid()
{
	UWorld * const world = GetWorld();
	check(world);
	const FVector2D totalGridSize = FVector2D(GridSize.X * CellSize.X, GridSize.Y * CellSize.Y);
	const FVector2D halfTotalGridSize = totalGridSize / 2.f;

	GridBoxComp->SetBoxExtent(FVector(halfTotalGridSize.X, halfTotalGridSize.Y, 50.f));

#if !SHIP_OR_TEST
	Debug_CreateGridText(); // Won't have region data, but that's fine
#endif
}

#if !SHIP_OR_TEST
void AKBGrid::Debug_CreateGridText()
{
	// Clear components first
	TInlineComponentArray<UTextRenderComponent*> textComponents(this);
	for (auto* component : textComponents)
	{
		if (IsValid(component))
		{
			component->DestroyComponent();
		}
	}

	const FVector actoLoc = GetActorLocation();

	for (int32 c = 0; c < GridSize.Y; ++c)
	{
		for (int32 r = 0; r < GridSize.X; ++r)
		{
			FGridRegion * region = GetCellRegion(FIntPoint(r, c));
			int32 idx = GridRegions.Find(*region);
			FColor textColor = FColor::White;
			if (idx != INDEX_NONE)
			{
				switch (idx)
				{
				case 0:
					textColor = FColor::Red;
					break;
				case 1:
					textColor = FColor::Green;
					break;
				case 2:
					textColor = FColor::Blue;
					break;
				case 3:
					textColor = FColor::Cyan;
					break;
				case 4:
					textColor = FColor::Magenta;
					break;
				case 5:
					textColor = FColor::Yellow;
					break;
				case 6:
					textColor = FColor::Turquoise;
					break;
				case 7:
					textColor = FColor::Purple;
					break;
				case 8:
					textColor = FColor::Orange;
					break;
				default:
					textColor = FColor::White;
					break;
				}
			}

			UTextRenderComponent * debugText = NewObject<UTextRenderComponent>(this);
			debugText->SetupAttachment(RootComponent);
			debugText->RegisterComponent();
			debugText->SetText(FText::FromString(FString::Printf(TEXT("%d,%d"), r, c)));
			debugText->SetRelativeRotation(FRotator(90.f, 123.690086, 33.689667));
			debugText->SetTextRenderColor(textColor);

			debugText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
			debugText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);

			FVector cellLoc = GetCellLocation(r, c);
			cellLoc.Z += 25.f;
			debugText->SetWorldLocation(cellLoc);
		}
	}

	// Grid region text
	for (int32 i = 0; i < GridRegions.Num(); ++i)
	{
		UTextRenderComponent * debugText = NewObject<UTextRenderComponent>(this);
		debugText->SetupAttachment(RootComponent);
		debugText->RegisterComponent();

		debugText->SetText(FText::FromString(FString::Printf(TEXT("Region #%d<br>Start: %d,%d<br>Dimensions: %d,%d"), i,
			GridRegions[i].StartPoint.X, GridRegions[i].StartPoint.Y,
			GridRegions[i].Dimensions.X, GridRegions[i].Dimensions.Y)));

		debugText->SetRelativeRotation(FRotator(90.f, 123.690086, 33.689667));
		debugText->SetTextRenderColor(FColor::Emerald);
		debugText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Left);
		debugText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextTop);

		FVector cellLoc = GetCellLocation(GridRegions[i].StartPoint.X, GridRegions[i].StartPoint.Y);
		cellLoc.Z += 125.f;
		debugText->SetWorldLocation(cellLoc);
	}


	//TArray<AKBWall*> walls;
	//TArray<AActor*> wallActors;
	//UGameplayStatics::GetAllActorsOfClass(this, AKBWall::StaticClass(), wallActors);
	//for (const auto& actor : wallActors)
	//{
	//	AKBWall * wall = Cast<AKBWall>(actor);
	//	if (IsValid(wall))
	//	{
	//		TInlineComponentArray<UTextRenderComponent*> wallTextComps(wall);
	//		for (auto* component : wallTextComps)
	//		{
	//			if (IsValid(component))
	//			{
	//				component->DestroyComponent();
	//			}
	//		}

	//		UTextRenderComponent * debugText = NewObject<UTextRenderComponent>(wall);
	//		debugText->SetupAttachment(wall->GetRootComponent());
	//		debugText->RegisterComponent();
	//		debugText->SetText(FText::FromString(FString::Printf(TEXT("%s (TargetLoc: %s)"), *wall->GetName(), *wall->GetTargetLocation().ToString())));
	//		debugText->SetRelativeRotation(wall->IsHorizontal() ? FRotator(90.f, 123.690086, 33.689667) : FRotator(90.f, 0.f, 0.f));
	//		FVector wallLoc = wall->GetActorLocation();
	//		wallLoc.Z += wall->WallCollision->GetScaledBoxExtent().Z + 10.f;
	//		debugText->SetWorldLocation(wallLoc);
	//		debugText->SetTextRenderColor(FColor::Turquoise);
	//		debugText->SetWorldScale3D(FVector::OneVector);
	//		debugText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	//		debugText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	//		debugText->SetWorldSize(16.f);
	//	}
	//}
}
#endif

void AKBGrid::InitializeGrid(bool firstTime /* = false */)//_Implementation()
{
	GridRegions.Empty();

	// Make our base region
	FGridRegion newRegion = FGridRegion(FIntPoint(0, 0), GridSize);
	GridRegions.Emplace(newRegion);

	TArray<AKBWall*> walls;
	TArray<AActor*> wallActors;
	//GetOverlappingActors(overlappingActors, AKBWall::StaticClass());
	UGameplayStatics::GetAllActorsOfClass(this, AKBWall::StaticClass(), wallActors);
	
	const FVector& boxExtent = GridBoxComp->GetScaledBoxExtent();
	
	// Pad by half cell size so walls on the very edge get caught by our reinitializations
	const FVector cellHalfSizes = FVector(CellSize.X * 0.5f, CellSize.Y * 0.5f, 0.f);
	const FVector mins = GetActorLocation() - boxExtent - cellHalfSizes;
	const FVector maxs = GetActorLocation() + boxExtent + cellHalfSizes;

	// TODO: Add a bool for auto create walls which exposes an array of wall start points sorted by priority

	for (const auto& actor : wallActors)
	{
		AKBWall * wall = Cast<AKBWall>(actor);
		if (IsValid(wall))
		{
			const FVector& wallLoc = wall->GetActorLocation();

			if (IsLocationWithinGrid(wallLoc, cellHalfSizes))
			{
				walls.Emplace(wall);
			}

			//if (wallLoc.X > mins.X && wallLoc.Y > mins.Y && wallLoc.X < maxs.X && wallLoc.Y < maxs.Y)
			//{
			//	walls.Emplace(wall);
			//}

			// TODO: im also gonna have to support region collapsing if i want this to be completely extensible,
			// i.e. 3x3 regions, bottom left (0,2) pushes wall all the way right, thus collapsing/eliminating region 1,2...
			//if(wallishorizontal && wall.x >= otherwall.x && wall.y == otherwall.y) ... something like that. might need wall dimensions
			//basically if walls overlap at all and are on same line, delete the lower priority, and reinitialize grid?
			//have to make sure to account for walls on the same line but NOT overlapping though.
		}
	}

	// Sorting backwards here since we do a reverse loop
	Algo::Sort(walls, [](const AKBWall * lhs, const AKBWall * rhs) { return lhs->WallPriority < rhs->WallPriority; });

	// Set the walls shape and position to make sure they're correct (the intent here is to let the level designs place walls as desired, but ensure all the settings are valid for our gameplay)
	for (int32 i = walls.Num() - 1; i >= 0; --i)
	{
		AKBWall * wall = walls[i];

		if (!IsValid(wall))
		{
			continue;
		}

		// Use wall target location as this function will also be used to reinitialize the regions as the wall is moving
		FIntPoint nearestCell = GetCellFromWorldLocation(wall->GetWallTargetLocation(), false, false);
		
		if (wall->IsWallAtEdge())
		{
			// Wall should already be destroyed at this point, but sanity
			continue;
		}

		const FVector wallTargetLoc = wall->GetWallTargetLocation();
		const FVector padding = FVector(CellSize.X * 0.2f, CellSize.Y * 0.2f, 0.f);
		
		// If the wall target loc is outside the grid, it likely had its region reduced to nothing, so remove it
		if (IsLocationWithinGrid(wallTargetLoc, padding) == false)
		{
			wall->Destroy();
			continue;
		}
		
		// Horizontal walls move vertical and vice versa
		const bool wallMovesVertically = wall->IsHorizontal();
		const bool atHorizontalLimit = !wallMovesVertically && (nearestCell.X >= GridSize.X || nearestCell.X <= 0);
		const bool atVerticalLimit = wallMovesVertically && (nearestCell.Y >= GridSize.Y || nearestCell.Y <= 0);
		if (atHorizontalLimit || atVerticalLimit)
		{
			nearestCell.X = FMath::Clamp(nearestCell.X, 0, GridSize.X);
			nearestCell.Y = FMath::Clamp(nearestCell.Y, 0, GridSize.Y);
			wall->SetWallAtEdge(true);
		}
		
		// Hack to support the fact that walls, sitting between cells, effectively have an extra possible grid location (i.e. with a row of 10 cells, there are 9 lines in between and 2 at either end of the row - walls sit on these lines)
		const FIntPoint hackyOffset = FIntPoint(nearestCell.X == GridSize.X ? -1 : 0, nearestCell.Y == GridSize.Y ? -1 : 0);

		FGridRegion* region = GetCellRegion(nearestCell + hackyOffset);
		
		if (!region)
		{
			wall->Destroy();
			continue;
		}

		// Account for our hacky boi, and starting at the corner of cells instead of the center
		const FIntPoint hackyStartPoint = region->StartPoint - hackyOffset;
		const FVector hackyCellSize = FVector(CellSize.X * 0.5f - hackyOffset.X * CellSize.X, CellSize.Y * 0.5f - hackyOffset.Y * CellSize.Y, 0.f);

		if (!wall->SplitEntireGrid)
		{
			const FVector regionStartLoc = GetCellLocation(hackyStartPoint) - hackyCellSize;
			wall->UpdateWallLocAndScale(nearestCell, CellSize, GetCellLocation(nearestCell), regionStartLoc, region, firstTime);
		}
		else
		{
			const FGridRegion modifiedRegion = FGridRegion(FIntPoint::ZeroValue, GridSize);
			const FIntPoint modifiedStartPoint = wall->IsHorizontal() ? FIntPoint(0, hackyStartPoint.Y) : FIntPoint(hackyStartPoint.X, 0);
			const FVector modifiedStart = GetCellLocation(modifiedStartPoint) - hackyCellSize;

			// Update wall with modified values to allow for it to be full sized either vertically or horizontally
			wall->UpdateWallLocAndScale(nearestCell, CellSize, GetCellLocation(nearestCell), modifiedStart, &modifiedRegion, firstTime);
		}

		if (firstTime)
		{
			wall->SetActorRotation(FRotator::ZeroRotator, ETeleportType::ResetPhysics); // We don't support wall rotation, make sure to zero it out (use actor x/y scale instead)
			wall->OnWallRequestMove.BindUObject(this, &AKBGrid::OnWallRequestedMove);
			wall->OnWallFinishedMoving.AddDynamic(this, &AKBGrid::OnWallFinishedMove);
		}

		if (!wall->SplitEntireGrid)
		{
			SplitRegion(region, nearestCell, wall);
		}
		else
		{
			// Reverse loop since we will be adding regions during this loop (but we won't need to access those)
			for (int32 idx = GridRegions.Num() - 1; idx >= 0; --idx)
			{
				// Wall is splitting the region it's in and all regions on its horizontal or vertical line
				if (wall->IsHorizontal() && nearestCell.Y > GridRegions[idx].StartPoint.Y && nearestCell.Y < GridRegions[idx].StartPoint.Y + GridRegions[idx].Dimensions.Y)
				{
					FIntPoint regionCell = nearestCell;
					regionCell.X = GridRegions[idx].StartPoint.X + FMath::FloorToInt(GridRegions[idx].Dimensions.X / 2);
					SplitRegion(&GridRegions[idx], regionCell, wall);
				}
				else if (!wall->IsHorizontal() && nearestCell.X > GridRegions[idx].StartPoint.X && nearestCell.X < GridRegions[idx].StartPoint.X + GridRegions[idx].Dimensions.X)
				{
					FIntPoint regionCell = nearestCell;
					regionCell.Y = GridRegions[idx].StartPoint.Y + FMath::FloorToInt(GridRegions[idx].Dimensions.Y / 2);
					SplitRegion(&GridRegions[idx], regionCell, wall);
				}
			}
		}
	}

#if !SHIP_OR_TEST
	Debug_CreateGridText();
#endif
	// also support dangling/manually placed box actors (i.e. on box beginplay if it wasnt spawned then snap it to grid - might need to do some deferred spawn thing for that)
}

FGridRegion* AKBGrid::GetCellRegion(const FIntPoint& cell) const
{
	// const_cast is bleh, but in lieu of not currently being able to edit engine code (see TArray::FindByPredicate) and also not wanting to make a chain of non-const getter functions, let's just use it
	FGridRegion* foundRegion = const_cast<FGridRegion*>(GridRegions.FindByPredicate([&cell](FGridRegion& currentRegion)
	{
		return currentRegion.IsCellWithinRegion(cell); 
	}));

	return foundRegion;
}

bool AKBGrid::IsSameRegion(const FIntPoint& cellA, const FIntPoint& cellB) const
{
	FGridRegion * const regionA = GetCellRegion(cellA);
	FGridRegion * const regionB = GetCellRegion(cellB);
	return regionA && regionB && regionA == regionB;
}

FGridRegion* AKBGrid::SplitRegion(FGridRegion* inRegion, const FIntPoint& splitPoint, AKBWall * const splittingWall)
{
	if (!ensure(inRegion)) { return nullptr; }

	FGridRegion outNewRegion = FGridRegion();
	
	if (splittingWall->IsHorizontal())
	{
		outNewRegion.StartPoint = FIntPoint(inRegion->StartPoint.X, splitPoint.Y);
		outNewRegion.Dimensions = FIntPoint(inRegion->Dimensions.X, inRegion->Dimensions.Y - splitPoint.Y);
		inRegion->Dimensions.Y = splitPoint.Y;
	}
	else
	{
		outNewRegion.StartPoint = FIntPoint(splitPoint.X, inRegion->StartPoint.Y);
		outNewRegion.Dimensions = FIntPoint(inRegion->Dimensions.X - splitPoint.X, inRegion->Dimensions.Y);
		inRegion->Dimensions.X = splitPoint.X;
	}

	if (outNewRegion.IsZeroRegion() == false)
	{
		// Update wall start cell
		splittingWall->SetNorthwestCell(outNewRegion.StartPoint);

		int32 idx = GridRegions.Emplace(outNewRegion);

		return &GridRegions[idx];
	}
	else
	{
		ensureAlwaysMsgf(false, TEXT("AKBGrid::SplitRegion - Failed to split region %s for point: %s"), splittingWall->IsHorizontal() ? TEXT("horizontally") : TEXT("vertically"), *splitPoint.ToString());
		return nullptr;
	}
}

FVector AKBGrid::GetCellLocation(int32 x, int32 y) const
{
	// TODO: These can be precalculated and reused
	const FVector2D totalGridSize = FVector2D(GridSize.X * CellSize.X, GridSize.Y * CellSize.Y);
	const FVector2D halfTotalGridSize = totalGridSize / 2.f;
	const FVector actoLoc = GetActorLocation();
	
	return  FVector(actoLoc.X + (CellSize.X * 0.5f) + (x * CellSize.X) - halfTotalGridSize.X,
					actoLoc.Y + (CellSize.Y * 0.5f) + (y * CellSize.Y) - halfTotalGridSize.Y,
					actoLoc.Z);
}

FVector AKBGrid::GetCellLocation(FIntPoint point) const
{
	return GetCellLocation(point.X, point.Y); // We rely on this allow out-of-bounds locations and returning valid data
}

void AKBGrid::SpawnSproutInEachRegion(TSubclassOf<AKBSprout> sproutClass)
{
	TArray<AActor*> charActors;
	UGameplayStatics::GetAllActorsOfClass(this, AKickboxersCharacter::StaticClass(), charActors);

	for (const auto& region : GridRegions)
	{
		TArray<FIntPoint> validCells;
		validCells.Reserve(region.CellCount());
		if (region.HasCells())
		{
			for (int32 i = 0; i < region.Dimensions.X; ++i) {
				for (int32 j = 0; j < region.Dimensions.Y; ++j) {
					validCells.Emplace(FIntPoint(region.StartPoint.X + i, region.StartPoint.Y + j));
				}
			}

			for (int32 i = validCells.Num() - 1; i >= 0; --i)
			{
				if (!IsCellClear(validCells[i], charActors) || GetSproutInCell(validCells[i]))
				{
					validCells.RemoveAt(i);
				}
			}
		}
		validCells.Shrink();

		if (validCells.Num() > 0)
		{
			FIntPoint spawnCell = validCells[FMath::RandRange(0, validCells.Num() - 1)];

			const FVector cellLocation = GetCellLocation(spawnCell);
			FTransform spawnTransform = FTransform(FRotator::ZeroRotator, cellLocation);
			FActorSpawnParameters params;
			params.Owner = this;

			AKBSprout * newSprout = GetWorld()->SpawnActor<AKBSprout>(sproutClass, spawnTransform, params);
		}
	}
}

void AKBGrid::AddGridComponent(UKBGridCellComponent * cellComp)
{
	CellComps.AddUnique(cellComp);
	//FIntPoint c = cellComp->GetGridCell();
	//if (IsCellsPointValid(c))
	//{
	//	Cells[c.X][c.Y].Comps.AddUnique(cellComp);
	//}
}

void AKBGrid::RemoveGridComponent(UKBGridCellComponent * cellComp)
{
	CellComps.Remove(cellComp);
	//FIntPoint c = cellComp->GetGridCell();
	//if (IsCellsPointValid(c))
	//{
	//	Cells[c.X][c.Y].Comps.Remove(cellComp);
	//}
}

void AKBGrid::NotifyGridComponentsOfPlayerMove(FIntPoint playerPrevCell)
{
	for (const auto& cell : CellComps)
	{
		if (cell->GetGridCell() == playerPrevCell)
		{
			cell->OnPlayerLeftCell.Broadcast();
		}
	}
}

void AKBGrid::SpawnRandomSproutInCell(FIntPoint cell)
{
	AKickboxersGameMode * gameMode = Cast<AKickboxersGameMode>(GetWorld()->GetAuthGameMode());
	if (!gameMode) { return; }

	const FVector cellLocation = GetCellLocation(cell);
	FTransform spawnTransform = FTransform(FRotator::ZeroRotator, cellLocation);
	FActorSpawnParameters params;
	params.Owner = this;

	TSubclassOf<AKBSprout> sproutClass = gameMode->GetRandomSproutClass();

	if (sproutClass)
	{
		AKBSprout * newSprout = GetWorld()->SpawnActor<AKBSprout>(gameMode->GetRandomSproutClass(), spawnTransform, params);
	}
}

void AKBGrid::SpawnRandomBoxInEachRegion(TSubclassOf<AKBBox> boxClass)
{
	ensure(HasAuthority());

	TArray<AActor*> charActors;
	UGameplayStatics::GetAllActorsOfClass(this, AKickboxersCharacter::StaticClass(), charActors);

	for (const auto& region : GridRegions)
	{
		TArray<FIntPoint> validCells;
		validCells.Reserve(region.CellCount());
		if (region.HasCells())
		{
			for (int32 i = 0; i < region.Dimensions.X; ++i) {
				for (int32 j = 0; j < region.Dimensions.Y; ++j) {
					validCells.Emplace(FIntPoint(region.StartPoint.X + i, region.StartPoint.Y + j));
				}
			}

			// Check for cells already filled with a box
			for (int32 i = validCells.Num() - 1; i >= 0; --i)
			{
				// Ignore the player here and do a check for their position manually, that way we are just checking their "point" instead of collision
				if (IsCellClear(validCells[i], charActors) == false)
				{
					validCells.RemoveAt(i);
				}
				else
				{
					for (const auto& actor : charActors)
					{
						if (validCells[i] == GetCellFromWorldLocation(actor->GetActorLocation()))
						{
							validCells.RemoveAt(i);
							break;
						}
					}
				}
			}
		}
		validCells.Shrink();

		int32 minimumEmptyCells = 3; // How many cells we want to be left empty (don't completely fill up the board)
		AKickboxersGameMode * gameMode = Cast<AKickboxersGameMode>(GetWorld()->GetAuthGameMode());
		if (gameMode)
		{
			minimumEmptyCells = gameMode->GetMinimumEmptyCells();
		}

		if (validCells.Num() > 0 && validCells.Num() > minimumEmptyCells)
		{
			// so, I wanted to spawn on the outside-most cells (so you can kick it at the wall), but I think a better
			// idea is simply prioritizing all inner cells (i.e. neither near the wall or at the edge)
			// So, iterate the validcells list, make a secondary list of inner cells, and then try to use that first

			// Another neat spawn rule here could be "try to not spawn directly cardinal to another box" - leaving
			// gaps for players to move in/out of and manipulate boxes better

			TArray<FIntPoint> interiorValidCells;
			interiorValidCells.Reserve(validCells.Num());
			for (int32 i = validCells.Num() - 1; i >= 0; --i)
			{
				if (validCells[i].X > region.StartPoint.X &&
					validCells[i].Y > region.StartPoint.Y &&
					validCells[i].X < (region.StartPoint.X + region.Dimensions.X - 1) &&
					validCells[i].Y < (region.StartPoint.Y + region.Dimensions.Y - 1))
				{
					interiorValidCells.Emplace(validCells[i]);
				}
			}
			interiorValidCells.Shrink();

			FIntPoint spawnCell;

			// Try to spawn on an interior cell if possible
			if (interiorValidCells.Num() > 0)
			{
				spawnCell = interiorValidCells[FMath::RandRange(0, interiorValidCells.Num() - 1)];
			}
			else
			{
				spawnCell = validCells[FMath::RandRange(0, validCells.Num() - 1)];
			}

			SpawnBoxInCell(spawnCell, boxClass, EBoxMoveType::SPAWN);
		}
	}
}	

AKBBox * AKBGrid::SpawnBoxInCell(FIntPoint cell, TSubclassOf<AKBBox> boxClass, EBoxMoveType spawnMoveType, AKickboxersCharacter * const optionalKicker /* = nullptr */)
{
	const FVector cellLocation = GetCellLocation(cell);
	FVector spawnLocation = cellLocation;

	if (spawnMoveType == EBoxMoveType::SPAWN)
	{
		spawnLocation.Z += BOX_MAX_SPAWN_HEIGHT - FMath::RandRange(0.f, 100.f); // Spawn in air above point
	}

	FTransform spawnTransform = FTransform(FRotator::ZeroRotator, spawnLocation);

	FActorSpawnParameters params;
	params.Owner = this;

	AKBBox * newBox = GetWorld()->SpawnActor<AKBBox>(boxClass, spawnTransform, params);
	if (IsValid(newBox))
	{
		if (optionalKicker)
		{
			newBox->SetKicker(optionalKicker);
		}

		newBox->SetupBox(this, cell, spawnMoveType);
	}

	return newBox;
}

bool AKBGrid::OnBoxPlaced(AKBBox * placedBox)
{
	TArray<AKBBox*> matchedBoxes;
	matchedBoxes.Add(placedBox);
	TArray<AKBBox*> allRegionBoxes = GetBoxesInRegion(placedBox->GetBoxCell());
	allRegionBoxes.Remove(placedBox);
	FindMatches(placedBox, allRegionBoxes, matchedBoxes);

	AKickboxersCharacter * const kicker = placedBox->GetKicker();

	const int32 BOXES_NEEDED_FOR_MATCH = 3;
	if (matchedBoxes.Num() >= BOXES_NEEDED_FOR_MATCH)
	{
		for (const auto& box : matchedBoxes)
		{
			box->SetComboNum(placedBox->GetComboNum() + 1);

			
			box->SetKicker(kicker);


			box->OnBoxMatched_Native();


		}

		OnBoxesMatchedDelegate.Broadcast(matchedBoxes, kicker);

		return true;
	}

	return false;

//	return matchedBoxes;
}

void AKBGrid::FindMatches(AKBBox* const currentBox, TArray<AKBBox*>& boxesToCheck, TArray<AKBBox*>& outBoxes) const
{
	AKBBox * northBox = FindBoxInNeighboringCell(currentBox, FIntPoint(0, -1), boxesToCheck);
	if (IsValid(northBox) && northBox->IsValidForMatches() && northBox->MatchesType(currentBox))
	{
		outBoxes.Add(northBox);
		boxesToCheck.Remove(northBox);
		FindMatches(northBox, boxesToCheck, outBoxes);
	}

	AKBBox * eastBox = FindBoxInNeighboringCell(currentBox, FIntPoint(1, 0), boxesToCheck);
	if (IsValid(eastBox) && eastBox->IsValidForMatches() && eastBox->MatchesType(currentBox))
	{
		outBoxes.Add(eastBox);
		boxesToCheck.Remove(eastBox);
		FindMatches(eastBox, boxesToCheck, outBoxes);
	}

	AKBBox * southBox = FindBoxInNeighboringCell(currentBox, FIntPoint(0, 1), boxesToCheck);
	if (IsValid(southBox) && southBox->IsValidForMatches() && southBox->MatchesType(currentBox))
	{
		outBoxes.Add(southBox);
		boxesToCheck.Remove(southBox);
		FindMatches(southBox, boxesToCheck, outBoxes);
	}

	AKBBox * westBox = FindBoxInNeighboringCell(currentBox, FIntPoint(-1, 0), boxesToCheck);
	if (IsValid(westBox) && westBox->IsValidForMatches() && westBox->MatchesType(currentBox))
	{
		outBoxes.Add(westBox);
		boxesToCheck.Remove(westBox);
		FindMatches(westBox, boxesToCheck, outBoxes);
	}
}

AKBBox * AKBGrid::FindBoxInNeighboringCell(AKBBox* const sourceBox, const FIntPoint& offset, const TArray<AKBBox*>& boxesToCheck) const
{
	AKBBox * const * neighborBoxPtr = boxesToCheck.FindByPredicate([&sourceBox, &offset](const AKBBox * iterBox)
	{
		if (!IsValid(iterBox)) return false;
		FIntPoint iterBoxCell = iterBox->GetBoxCell();
		return (iterBox->GetBoxCell().X == sourceBox->GetBoxCell().X + offset.X &&
				iterBox->GetBoxCell().Y == sourceBox->GetBoxCell().Y + offset.Y &&
				iterBox->IsBoxOnFloor()); // If I ever end up supporting stacked boxes, this function for now should just find the bottom-most box
	});

	// Need to check for null as the null double ptr is to be expected often
	if (neighborBoxPtr != nullptr)
	{
		return *neighborBoxPtr;
	}
	else
	{
		return nullptr;
	}
}

AKBBox * AKBGrid::GetBoxInCell(const FIntPoint& cell) const
{
	TArray<AActor*> boxActors;
	UGameplayStatics::GetAllActorsOfClass(this, AKBBox::StaticClass(), boxActors);

	for (const auto& ba : boxActors)
	{
		AKBBox * const box = Cast<AKBBox>(ba);
		if (IsValid(box) && box->GetBoxCell() == cell)
		{
			return box;
		}
	}

	return nullptr;
}

AKBSprout * AKBGrid::GetSproutInCell(const FIntPoint& cell) const
{
	for (TActorIterator<AKBSprout> it(GetWorld()); it; ++it)
	{
		AKBSprout * const sprout = *it;

		// TODO: this sucks
		if (IsValid(sprout) && sprout->FindComponentByClass<UKBGridCellComponent>()->GetGridCell() == cell)
		{
			return sprout;
		}
	}

	return nullptr;
}

TArray<AKBBox*> AKBGrid::GetBoxesInRegion(const FGridRegion* const region) const
{
	TArray<AKBBox*> boxes;

	TArray<AActor*> boxActors;
	UGameplayStatics::GetAllActorsOfClass(this, AKBBox::StaticClass(), boxActors);

	for (const auto& a : boxActors)
	{
		AKBBox * const box = Cast<AKBBox>(a);
		if (IsValid(box))
		{
			if (GetCellRegion(box->GetBoxCell()) == region)
			{
				boxes.Emplace(box);
			}
		}
	}

	return boxes;
}

TArray<AKBBox*> AKBGrid::GetBoxesInRegion(const FIntPoint& sourceCell) const
{
	FGridRegion* const region = GetCellRegion(sourceCell);
	return GetBoxesInRegion(region);
}

FIntPoint AKBGrid::GetCellFromWorldLocation(FVector loc, bool clampToGridBounds /* = true */, bool floor /* = true */) const
{
	if (!ensure(CellSize.X != 0 && CellSize.Y != 0))
	{
		return FIntPoint::NoneValue;
	}

	const FVector totalGridSize = FVector(GridSize.X * CellSize.X, GridSize.Y * CellSize.Y, 0.f);
	const FVector halfTotalGridSize = totalGridSize / 2.f;
	FVector actorLoc = GetActorLocation();
	actorLoc.Z = 0.f; // In case Z ever matters - we shouldn't care about it here

	const FVector gridStartLoc = actorLoc - halfTotalGridSize;
	const FVector adjustedLoc = loc - gridStartLoc;

	FIntPoint gridPoint;
	if (floor)
	{
		gridPoint = FIntPoint(FMath::FloorToInt(adjustedLoc.X / CellSize.X),
							  FMath::FloorToInt(adjustedLoc.Y / CellSize.Y));
	}
	else
	{
		gridPoint = FIntPoint(FMath::RoundToInt(adjustedLoc.X / CellSize.X),
							  FMath::RoundToInt(adjustedLoc.Y / CellSize.Y));
	}

	if (clampToGridBounds)
	{
		gridPoint.X = FMath::Clamp(gridPoint.X, 0, GridSize.X - 1);
		gridPoint.Y = FMath::Clamp(gridPoint.Y, 0, GridSize.Y - 1);
	}

	return gridPoint;
}

bool AKBGrid::IsPointWithinGrid(FIntPoint point) const
{
	return (point.X >= 0 && point.Y >= 0 && point.X < GridSize.X && point.Y < GridSize.Y);
}

bool AKBGrid::IsLocationWithinGrid(const FVector& location, const FVector& padding /* = FVector::ZeroVector */) const
{
	const FVector gridLoc = GetActorLocation();
	const FVector gridExtent = FVector(GridSize.X * CellSize.X * 0.5f, GridSize.Y * CellSize.Y * 0.5f, 0.f);
	const FVector mins = gridLoc - gridExtent - padding;
	const FVector maxs = gridLoc + gridExtent + padding;

	return (location.X >= mins.X && location.Y >= mins.Y && location.X <= maxs.X && location.Y <= maxs.Y);
}

void AKBGrid::OnWallRequestedMove(AKBWall * const movingWall, ECardinalDirection dir)
{
	for (int32 i = WallMoveQueue.Num() - 1; i >= 0; --i)
	{
		if (WallMoveQueue[i].Wall == movingWall && WallMoveQueue[i].Direction == CardinalOpposite(dir))
		{
			// Already have the same wall queued to move in the opposite direction - just remove the queued move
			WallMoveQueue.RemoveAt(i);
			return;
		}
	}

	
	FWallQueuedMove wallMove;
	wallMove.Wall = movingWall;
	wallMove.Direction = dir;

	WallMoveQueue.Emplace(wallMove);

	//TRACEMSG("[%s] OnWallRequestedMove, new queue added: %s going %s", *ENUM_AS_STRING(ENetRole, GetLocalRole()), *movingWall->GetName(), *ENUM_AS_STRING(ECardinalDirection, dir));

	// Check if any wall is moving ... TODO: this should only check this grid's walls? on grid init, store tied walls?
	bool anyWallMoving = false;
	TArray<AActor*> actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AKBWall::StaticClass(), actors);
	for (const auto& a : actors)
	{
		AKBWall * wall = Cast<AKBWall>(a);
		if (IsValid(wall) && wall->IsWallMoving())
		{
			anyWallMoving = true;
			break;
		}
	}

	if (!anyWallMoving)
	{
		// No walls currently moving, do next move immediately
		DoNextWallMove();
	}
	else
	{
		// If a wall is moving, when it ends it should trigger the next move automatically via its WallFinishedMoving delegate (intentionally blank)
	}
}

void AKBGrid::DoNextWallMove()
{
	const float WallMoveDuration = 0.5f;

	if (WallMoveQueue.Num() > 0)
	{
		//TRACEMSG("[%s] DoNextWallMove: %s going %s", *ENUM_AS_STRING(ENetRole, GetLocalRole()), *WallMoveQueue[0].Wall->GetName(), *ENUM_AS_STRING(ECardinalDirection, WallMoveQueue[0].Direction));
		TArray<AKBBox*> boxesToMove;
		boxesToMove.Reserve(GridSize.X * GridSize.Y);

		FWallQueuedMove move = WallMoveQueue[0];
		if (IsValid(move.Wall))
		{
			move.Wall->SetWallTargetLocation(move.Wall->GetWallTargetLocation() + (CardinalToVector(move.Direction) * GetCellDimension(IsDirectionHorizontal(move.Direction))));
			
			TArray<AActor*> actors = move.Wall->GetNeighboringActors(move.Direction, this);
			for (const auto& actor : actors)
			{
				// Move all neighboring walls
				AKBWall * const wall = Cast<AKBWall>(actor);
				if (IsValid(wall))
				{
					wall->SetWallTargetLocation(wall->GetWallTargetLocation() + (CardinalToVector(move.Direction) * GetCellDimension(IsDirectionHorizontal(move.Direction))));
				}
				else
				{
					// Move all neighboring boxes
					AKBBox * const box = Cast<AKBBox>(actor);
					if (IsValid(box))
					{
						box->RecursiveBoxMove(move.Direction, boxesToMove);
					}
					else
					{
						AKickboxersCharacter * character = Cast<AKickboxersCharacter>(actor);
						if(IsValid(character))
						{
							// Didn't hit a box, but did hit a character, push them away
							character->PushCharacterAway(move.Direction);

							// Do another trace to see if there's boxes in their path and push those too
							FCollisionQueryParams params;
							params.AddIgnoredActor(character);
							
							const FVector boxMoveDir = CardinalToVector(move.Direction) * GetCellDimension(IsDirectionHorizontal(move.Direction));
							const FVector traceStart = character->GetActorLocation() + FVector(0.f, 0.f, -20.f) + (boxMoveDir * 0.5f);
							const FVector traceEnd = traceStart + (boxMoveDir * 0.5f);
							const float charRadius = character->GetCapsuleComponent()->GetScaledCapsuleRadius();
							TArray<FHitResult> boxHits;
							FCollisionShape shape = FCollisionShape::MakeBox(FVector(charRadius, charRadius, character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
							//GetWorld()->bDebugDrawAllTraceTags = true;
							GetWorld()->SweepMultiByChannel(boxHits, traceStart, traceEnd, FQuat::Identity, ECollisionChannel::ECC_Pawn, shape, params);
							//GetWorld()->bDebugDrawAllTraceTags = false;

							for (const auto& hit : boxHits)
							{
								AKBBox * const boxBehindPlayer = Cast<AKBBox>(hit.GetActor());
								if (IsValid(boxBehindPlayer))
								{
									// Hit a box behind the player - recursively move that row too so the player doesn't get stuck
									boxBehindPlayer->RecursiveBoxMove(move.Direction, boxesToMove);
								}
							}
						}
					}
				}
			}
		}

		if (HasAuthority()) // Should be redundant
		{
			TArray<FVector> locArray;
			locArray.Reserve(boxesToMove.Num());
			// get box locations here and pass as a separate array
			for (int32 i = 0; i < boxesToMove.Num(); ++i)
			{
				locArray.Emplace(boxesToMove[i]->FinalTargetLocation);
			}

			// Apply the move to boxes in one lump transaction to try and get them to sync up (kind of a hack)
			ApplyWallMoveToBoxes_Multicast(boxesToMove, locArray);
		}

		WallMoveQueue.RemoveAt(0);

		InitializeGrid(); // Reinitialize grid for new regions and actually moving the wall actors
	}
}

void AKBGrid::ApplyWallMoveToBoxes_Multicast_Implementation(const TArray<AKBBox*>& boxes, const TArray<FVector>& locations)
{
	for(int32 i = 0; i < boxes.Num(); ++i)
	{
		if (IsValid(boxes[i]) && locations.IsValidIndex(i)) // Sanity check, it sucks to have to pass 2 arrays but TMap replication not supported
		{
			boxes[i]->SetBoxTargetLocation(locations[i]);
		}
	}
}

void AKBGrid::OnWallFinishedMove(AKBWall * const wall)
{
	bool found = false;

	//TRACEMSG("[%s] OnWallFinishedMove (wall: %s), num WallMoveQueue remaining: %d", *ENUM_AS_STRING(ENetRole, GetLocalRole()), *wall->GetName(), WallMoveQueue.Num());

	if (IsValid(wall) && wall->IsWallAtEdge())
	{
		// Remove all moves for this wall if it's at the edge, as it'll be destroyed
		for(int32 i = WallMoveQueue.Num() - 1; i >= 0; --i)
		{
			if (WallMoveQueue[i].Wall == wall)
			{
				WallMoveQueue.RemoveAt(i);
			}
		}
	}

	if (WallMoveQueue.Num() > 0)
	{
		DoNextWallMove();
	}
}

bool AKBGrid::IsCellClear(FIntPoint cell) const
{
	return IsCellClear(cell, {});
}

bool AKBGrid::IsCellClear(FIntPoint cell, TArray<AActor*> actorsToIgnore) const
{
	AActor * outActor;
	return IsCellClear(cell, actorsToIgnore, outActor);
}

bool AKBGrid::IsCellClear(FIntPoint cell, TArray<AActor*> actorsToIgnore, AActor *& outBlockingActor) const
{
	// @TODO - use grid components that this grid is aware of instead of just boxes
	// Check if we have any blocking actors that are assigned to this grid space
	//for (const auto& actor : GetGridActorsInCell(cell))
	//{
	//	UKBGridCellComponent * gridComp = actor->FindComponent
			// maybe this should loop thru the components directly
			// but anyway what we want to do here is check if they're blocking
			// loop thru components via gridcomponent and check for collision channels
			// if collision enabled and collision response to channel block
	//}

	if (GetBoxInCell(cell))
	{
		// Do an early check to see if a box is in the cell, then we can avoid the sweep
		return false;
	}

	FCollisionQueryParams collisionQueryParams;
	collisionQueryParams.bIgnoreBlocks = false;
	collisionQueryParams.AddIgnoredActor(this);

	for (const auto& actor : actorsToIgnore)
	{
		collisionQueryParams.AddIgnoredActor(actor);
	}

	FVector shapeExtent = FVector(CellSize.X * 0.3f, CellSize.Y * 0.3f, 40.f);

	FCollisionShape shape = FCollisionShape::MakeBox(shapeExtent);
	
	FVector traceLoc = GetCellLocation(cell);
	traceLoc.Z += 50.f;
	
	//DrawDebugBox(GetWorld(), traceLoc, shapeExtent, FQuat::Identity, FColor::Red, false, 3.0f, 0, 2.f);

	FHitResult hit;

	// Check for pawn here instead of box - pawn should be blocked by anything a box should be typically but will catch more cases
	bool didHit = GetWorld()->SweepSingleByChannel(hit, traceLoc, traceLoc, FQuat::Identity, ECollisionChannel::ECC_Pawn, shape, collisionQueryParams); //GetWorld()->SweepMultiByChannel(hits, traceLoc, traceLoc, FQuat::Identity, ECollisionChannel::ECC_Pawn, shape, collisionQueryParams);

	if (hit.Actor.IsValid())
	{
		outBlockingActor = hit.Actor.Get();
	}

	return !didHit;
}

void AKBGrid::CheckStuckCharacters_Server_Implementation()
{
	TArray<AActor*> charActors;
	UGameplayStatics::GetAllActorsOfClass(this, AKickboxersCharacter::StaticClass(), charActors);

	for (const auto& actor : charActors)
	{
		AKickboxersCharacter * character = Cast<AKickboxersCharacter>(actor);
		if (IsValid(character))
		{
			const FIntPoint charCell = GetCellFromWorldLocation(character->GetActorLocation());
			FGridRegion * const charRegion = GetCellRegion(charCell);

			if (charRegion == nullptr)
			{
				// If we have no region, don't bother checking stuck
				continue;
			}

			// Check each cardinal dir and do a test kick to see if the boxes will move
			bool hasAnyValidMoves = false;

			// TODO, improve this with a new function - each of these is a get all actors check...
			TMap<ECardinalDirection, AKBBox*> boxMap;
			boxMap.Reserve(4);
			boxMap.Add(ECardinalDirection::NORTH, GetBoxInCell(charCell + UKBUtility::CardinalToPoint(ECardinalDirection::NORTH)));
			boxMap.Add(ECardinalDirection::EAST, GetBoxInCell(charCell + UKBUtility::CardinalToPoint(ECardinalDirection::EAST)));
			boxMap.Add(ECardinalDirection::SOUTH, GetBoxInCell(charCell + UKBUtility::CardinalToPoint(ECardinalDirection::SOUTH)));
			boxMap.Add(ECardinalDirection::WEST, GetBoxInCell(charCell + UKBUtility::CardinalToPoint(ECardinalDirection::WEST)));

			for (const auto& box : boxMap)
			{
				if (IsValid(box.Value))
				{
					FIntPoint tempPoint;
					EBoxMoveType tempType;

					if (box.Value->GetBoxMoveForDir(box.Key, character, tempPoint, tempType))
					{
						hasAnyValidMoves = true;
						break;
					}
				}
				else
				{
					const FIntPoint pointToCheck = charCell + UKBUtility::CardinalToPoint(box.Key);
					
					// No box at this cell, check if the cell is clear of obstacles
					if (charRegion->IsCellWithinRegion(pointToCheck) && IsCellClear(pointToCheck, charActors)) // Ignore all characters from this check
					{
						// Cell is valid and clear - we're good
						hasAnyValidMoves = true;
						break;
					}
				}
			}

			if (!hasAnyValidMoves)
			{
				character->OnCharacterStuck();
			}
			else if (character->IsCharacterStuck())
			{
				character->OnCharacterUnstuck();
			}
		}
	}
}
