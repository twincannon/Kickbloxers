// Fill out your copyright notice in the Description page of Project Settings.


#include "KBBox.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kickboxers.h"
#include "DrawDebugHelpers.h"
#include "Actors/KBGrid.h"
#include "Actors/KBWall.h"
#include "KickboxersCharacter.h"
#include "KBUtility.h"
#include "KBPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Components/KBGridCellComponent.h"
#include "KBGameState.h"

// Sets default values
AKBBox::AKBBox()
{

	BoxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BoxRoot"));
	RootComponent = BoxRoot;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	BoxCollision->SetRelativeLocation(FVector(0.f, 0.f, BoxCollision->GetScaledBoxExtent().Z));
	BoxCollision->SetCollisionObjectType(COLLISION_BOX);
	BoxCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	BoxCollision->SetCollisionResponseToChannel(COLLISION_BOX, ECollisionResponse::ECR_Block);
	BoxCollision->SetCollisionResponseToChannel(COLLISION_GRID, ECollisionResponse::ECR_Overlap);
	BoxCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	BoxCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	BoxCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Ignore);
	BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &AKBBox::OnBoxBeginOverlap);
	BoxCollision->OnComponentHit.AddDynamic(this, &AKBBox::OnBoxHit);
	BoxCollision->SetupAttachment(RootComponent);

	// Create player check collision to overlap when the player is underneath a falling box
	PlayerCheck = CreateDefaultSubobject<UBoxComponent>(TEXT("PlayerCheck"));
	PlayerCheck->SetBoxExtent(FVector(20.f, 20.f, 10.f)); // Should be smaller than the main collision box
	PlayerCheck->SetRelativeLocation(FVector(0.f, 0.f, -PlayerCheck->GetScaledBoxExtent().Z));
	PlayerCheck->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PlayerCheck->SetCollisionObjectType(COLLISION_BOX);
	PlayerCheck->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	PlayerCheck->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	PlayerCheck->OnComponentBeginOverlap.AddDynamic(this, &AKBBox::OnPlayerCheckOverlap);
	PlayerCheck->SetupAttachment(RootComponent);

	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> boxMeshAsset(TEXT("/Game/Geometry/Meshes/1M_Cube_Chamfer.1M_Cube_Chamfer"));
	if (boxMeshAsset.Succeeded())
	{
		BoxMesh->SetStaticMesh(boxMeshAsset.Object);
	}
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxMesh->SetupAttachment(BoxCollision);

#if !SHIP_OR_TEST
	DebugText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DebugText"));
	DebugText->SetRelativeLocationAndRotation(FVector(0.f, 0.f, 52.f), FRotator(90.f, 123.690086, 33.689667));
	DebugText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	DebugText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	DebugText->SetupAttachment(BoxCollision);
#endif

	GridComponent = CreateDefaultSubobject<UKBGridCellComponent>(TEXT("GridComponent"));

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bOnlyRelevantToOwner = false;
	bAlwaysRelevant = true;
	bNetLoadOnClient = true;

	// SetReplicateMovement(true); // seems like this is only for CMC and actor physics
}

// Called when the game starts or when spawned
void AKBBox::BeginPlay()
{
	TargetLocation = GetActorLocation();

	Super::BeginPlay();

	OnBoxFinishedMoving.AddDynamic(this, &AKBBox::OnBoxFinishMove);

	if (HasAuthority())
	{
		HeadBounceDirs =
		{
			FIntPoint(UKBUtility::CardinalToPoint(NORTH)),
			FIntPoint(UKBUtility::CardinalToPoint(NORTH) + UKBUtility::CardinalToPoint(EAST)),
			FIntPoint(UKBUtility::CardinalToPoint(EAST)),
			FIntPoint(UKBUtility::CardinalToPoint(EAST) + UKBUtility::CardinalToPoint(SOUTH)),
			FIntPoint(UKBUtility::CardinalToPoint(SOUTH)),
			FIntPoint(UKBUtility::CardinalToPoint(SOUTH) + UKBUtility::CardinalToPoint(WEST)),
			FIntPoint(UKBUtility::CardinalToPoint(WEST)),
			FIntPoint(UKBUtility::CardinalToPoint(WEST) + UKBUtility::CardinalToPoint(NORTH)),
		};

		ShuffleHeadBounceDirs();
	}
}

void AKBBox::ShuffleHeadBounceDirs()
{
	int32 LastIndex = HeadBounceDirs.Num() - 1;
	for (int32 i = 0; i <= LastIndex; ++i)
	{
		int32 Index = FMath::RandRange(i, LastIndex);
		if (i != Index)
		{
			HeadBounceDirs.Swap(i, Index);
		}
	}
}

void AKBBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKBBox, TargetLocation);
	DOREPLIFETIME(AKBBox, CurrentMoveType);
}

//void AKBBox::OnRep_BoxCell(const FIntPoint& oldCell)
//{
//	// If you feed a RepNotify function a ref param of the same type as is being replicated, it will pass
//	// the old value of the variable. However it is the servers old value, not the clients - and we need
//	// to check what the client set itself and delta that with what the server sent the client to check
//	// for errors
//
//	// BoxCell has now been replicated to this client so check it against our client copy variable
//	//if (BoxCell_Shadow != BoxCell)
//
//#if !SHIP_OR_TEST
//	DebugText->SetText(FText::FromString(FString::Printf(TEXT("%d,%d"), BoxCell.X, BoxCell.Y)));
//#endif
//}

void AKBBox::SetupBox_Implementation(AKBGrid * grid, FIntPoint startPoint, EBoxMoveType initialMoveType)
{
	ensureAlways(grid);

	SetBoxCell(startPoint);

	if (initialMoveType == EBoxMoveType::SPAWN)
	{
		const FVector startLoc = grid->GetCellLocation(startPoint);
		PlaySpawnAnimation(startLoc);
	}
	else if (initialMoveType == EBoxMoveType::TELEPORT)
	{
		const FVector startLoc = grid->GetCellLocation(startPoint);
		SetActorLocation(startLoc);
		SetBoxTargetLocation(startLoc);
	}
	else
	{
		MoveBoxTo_Internal(FIntPoint(startPoint.X, startPoint.Y), initialMoveType);
	}
}

// Called every frame
void AKBBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		// Set replicated TargetLocation to our actor location (which is typically updated via blueprint MoveBoxTo)
		TargetLocation = GetActorLocation();
	}
	else
	{
		//SetActorLocation(TargetLocation);
		const FVector& actorLoc = GetActorLocation();
		if (!actorLoc.Equals(TargetLocation))
		{
			const float lerpSmoothness = 0.3f;
			SetActorLocation(FMath::Lerp(actorLoc, TargetLocation, lerpSmoothness));
		}
	}

}

void AKBBox::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComaponent, class AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	// Box overlapped with grid
}

void AKBBox::OnBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Box hit player or world
}

void AKBBox::OnPlayerCheckOverlap(UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	// Disabling bounce

	//if (HasAuthority())
	//{
	//	AKickboxersCharacter * character = Cast<AKickboxersCharacter>(OtherActor);
	//	if (IsValid(character))
	//	{
	//		// If we hit this, that means the box is falling on the players head, which can cause bad things, try to move to a nearby cell
	//		if (AKBGrid * const grid = GridComponent->GetGrid())
	//		{
	//			FGridRegion * const region = grid->GetCellRegion(GetBoxCell());
	//			if (region)
	//			{
	//				// Check all cardinal and intercardinal directions... if they're all filled then how the hell did the player even get here?
	//				bool foundCell = false;
	//				int32 idx = 0;
	//				for (; idx < HeadBounceDirs.Num(); ++idx)
	//				{
	//					FIntPoint newPoint = GetBoxCell() + HeadBounceDirs[idx];

	//					if (region->IsCellWithinRegion(newPoint) && grid->IsCellClear(newPoint))
	//					{
	//						//Kicker.Reset(); // Clear kicker so bounces don't cause matches as they're random
	//						MoveBoxTo_Internal(newPoint, EBoxMoveType::BOUNCE);

	//						foundCell = true;

	//						ShuffleHeadBounceDirs();
	//						return;
	//					}
	//				}

	//				// Failed to head bounce - we're probably killing the player at this point, so just let it happen baby
	//			}
	//		}
	//	}
	//}
}

bool AKBBox::CanBeKicked() const
{
	return GridComponent->HasValidCell();
}

void AKBBox::KickBox(ECardinalDirection pushDir, AKickboxersCharacter * const pusher)
{
	if (!CanBeMoved()) { return; }

	if (AKBGrid * const grid = GridComponent->GetGrid())
	{
		SetKicker(pusher);
		EBoxMoveType moveType = EBoxMoveType::DEFAULT;
		FIntPoint moveToPoint = FIntPoint::NoneValue;

		bool canMove = GetBoxMoveForDir(pushDir, pusher, moveToPoint, moveType);

		if (GetBoxMoveForDir(pushDir, pusher, moveToPoint, moveType))
		{
			if (moveToPoint != GetBoxCell())
			{
				MoveBoxTo_Internal(moveToPoint, moveType);
			}
		}
		else
		{
			// Try pushing the box and anything blocking it
			KickBoxRow(pushDir, pusher);
		}
	}
}

void AKBBox::SetKicker(AKickboxersCharacter * const newKicker)
{
	Kicker = newKicker;
}

bool AKBBox::GetBoxMoveForDir(ECardinalDirection pushDir, AKickboxersCharacter * const pusher, FIntPoint &outMoveToPoint, EBoxMoveType &outMoveType) const
{
	AKBGrid * const grid = GridComponent->GetGrid();
	if (!grid)
	{
		return false; // No grid, no move
	}

	if (NumTilesPerMove <= 0)
	{
		return false;
	}

	// Do a trace for blocking actors to see where the box should be placed
	FCollisionQueryParams collisionQueryParams;
	collisionQueryParams.bIgnoreBlocks = false;
	collisionQueryParams.AddIgnoredActor(this);
	//collisionQueryParams.AddIgnoredActor(pusher);

	FCollisionShape shape = FCollisionShape::MakeBox(FVector(20.f));

	FHitResult hit;
	FVector traceStart = GetActorLocation();
	traceStart.Z += BoxCollision->GetScaledBoxExtent().Z * 0.5f;
	const float cellDimension = grid->GetCellDimension(IsDirectionHorizontal(pushDir));
	FVector traceEnd = traceStart + CardinalToVector(pushDir) * (NumTilesPerMove * cellDimension);

	GetWorld()->SweepSingleByChannel(hit, traceStart, traceEnd, FQuat::Identity, COLLISION_BOX, shape, collisionQueryParams);

	if (IsValid(hit.GetActor()))
	{
		//FIntPoint actorCell = grid->GetCellFromWorldLocation(hit.GetActor()->GetActorLocation());
		UKBGridCellComponent * otherGridComp = hit.GetActor()->FindComponentByClass<UKBGridCellComponent>();
		if (IsValid(otherGridComp))
		{
			FIntPoint actorCurrentCell = grid->GetCellFromWorldLocation(hit.GetActor()->GetActorLocation());
			FIntPoint actorGridCell = otherGridComp->GetGridCell();

			// Ok this is confusing so for now just always use their target grid cell
			outMoveToPoint = actorGridCell + UKBUtility::CardinalToPoint(CardinalOpposite(pushDir));

			//if (actorCurrentCell == actorGridCell)
			//{
			//	outMoveToPoint = actorGridCell + UKBUtility::CardinalToPoint(CardinalOpposite(pushDir));
			//}
			//else
			//{
			//	// Player is moving from one cell to another, account for their target position


			//	FIntPoint traceEndCell = grid->GetCellFromWorldLocation(traceEnd, true);

			//	// Figure out if we should stop at the actor's current or target cell, and step back 1 from that
			//	if (FMath::Abs(actorGridCell.X) < FMath::Abs(actorCurrentCell.X) ||
			//		FMath::Abs(actorGridCell.Y) < FMath::Abs(actorCurrentCell.Y))
			//	{
			//		outMoveToPoint = actorGridCell + UKBUtility::CardinalToPoint(CardinalOpposite(pushDir));
			//	}
			//	else
			//	{
			//		outMoveToPoint = actorCurrentCell + UKBUtility::CardinalToPoint(CardinalOpposite(pushDir));
			//	}

			//	if (outMoveToPoint == GetBoxCell())
			//	{
			//		// Hacky way to avoid edge case weirdness when a player pushes a row of blocks that results in a block being blown back towards them: [a][a]__[a][b]<-[player]
			//		// (we don't want to hit the end of this function where this case is a false return)
			//		return true;
			//	}
			//}
		}
		else
		{
			const FVector2D cellSize = grid->GetCellSize();
			FVector cellSize3D = FVector(cellSize.X, cellSize.Y, 0.f);
			FVector adjustedImpact = hit.ImpactPoint;
			adjustedImpact -= CardinalToVector(pushDir) * cellSize3D * 0.5f;

			outMoveToPoint = grid->GetCellFromWorldLocation(adjustedImpact);
		}
	}
	else
	{
		// Didn't hit any actor, so just go to the end of the grid in that direction
		outMoveToPoint = grid->GetCellFromWorldLocation(traceEnd, true);
	}

	if (outMoveToPoint != FIntPoint::NoneValue)
	{
		// See if we're in a spot where a box is going to be (box spawning in, still falling) and adjust if necessary
		if (outMoveToPoint != GetBoxCell())
		{
			// Only do this if we're moving cells, otherwise the box could move backwards
			FIntPoint adjustedPoint = outMoveToPoint;
			bool doAdjustment = false;
			while (grid->GetBoxInCell(adjustedPoint) != nullptr)
			{
				adjustedPoint += (UKBUtility::CardinalToPoint(pushDir) * -1);
				doAdjustment = true;

				if (adjustedPoint == GetBoxCell())
				{
					// Stepped back so far that the box is at the same point - going backwards doesn't make sense, so break
					break;
				}

				if (IsValid(pusher))
				{
					FIntPoint playerCell = grid->GetCellFromWorldLocation(pusher->GetActorLocation(), false);
					if (grid->IsPointWithinGrid(adjustedPoint) == false || playerCell == adjustedPoint)
					{
						// Trying to move onto/through the player (shouldn't be possible), or off the grid - abort
						doAdjustment = false;
						break;
					}
				}
			}

			if (doAdjustment)
			{
				outMoveToPoint = adjustedPoint;
			}
		}

		// Box didn't move - check to see if we should do some kind of alternate move
		if (outMoveToPoint == GetBoxCell())
		{
			FGridRegion * const thisBoxRegion = grid->GetCellRegion(GetBoxCell());
			if (CanBeFlipped)
			{
				// Previous point direction -2 == behind player
				const FIntPoint flipPoint = outMoveToPoint + (UKBUtility::CardinalToPoint(pushDir) * -2);
				if (grid->IsPointWithinGrid(flipPoint) && grid->IsCellClear(flipPoint))
				{
					// Make sure it's in the same region so we don't jump over walls (this also fixes going out of bounds)
					if (thisBoxRegion == grid->GetCellRegion(flipPoint))
					{
						outMoveToPoint = flipPoint;
						outMoveType = EBoxMoveType::FLIP;
					}
				}
			}
		}
	}

	// Is this a new point that is also within the grid?
	return (outMoveToPoint != GetBoxCell() && grid->IsPointWithinGrid(outMoveToPoint));
}

void AKBBox::KickBoxRow(ECardinalDirection pushDir, AKickboxersCharacter * const pusher)
{
	// @TODO: This function needs to account for the fact that the characters cell is not changing (i.e. if you kick a flippable box it needs to not flip onto the characters cell)
	PushBoxRow(pushDir, pusher);
}

bool AKBBox::PushBoxRow(ECardinalDirection pushDir, AKickboxersCharacter * const pusher)
{
	if (!CanBeMoved()) { return false; }

	AKBGrid * const grid = GridComponent->GetGrid();
	if (!ensure(grid)) { return false; }

	const FIntPoint pushOffset = UKBUtility::CardinalToPoint(pushDir);

	TArray<AKBBox*> boxes;
	boxes.Emplace(this);
	int32 offsetIndex = 1;

	bool boxesBlocked = false;
	bool endedInSameRegion = false;

	FIntPoint targetCell = GetBoxCell() + pushOffset;
	while (grid->IsSameRegion(GetBoxCell(), targetCell))
	{
		endedInSameRegion = true;

		if (!grid->IsCellClear(targetCell))
		{
			AKBBox * box = grid->GetBoxInCell(targetCell);
			if (IsValid(box))
			{
				boxes.Emplace(box);
			}
			else
			{
				// Not clear and not a box
				boxesBlocked = true;
				break;
			}
		}
		else
		{
			// Clear cell, no more pushing to be done
			break;
		}

		offsetIndex++;
		targetCell = GetBoxCell() + (pushOffset * offsetIndex);

		endedInSameRegion = false;
	}

	if (!endedInSameRegion)
	{
		boxesBlocked = true;
	}

	for (int32 i = boxes.Num() - 1; i >= 0; --i)
	{
		if (!boxesBlocked)
		{
			if (boxes[i]->CanBeMoved())
			{
				boxes[i]->Kicker = pusher;
				boxes[i]->MoveBoxTo_Internal(boxes[i]->GetBoxCell() + pushOffset);
			}
			else
			{
				// Boxes are blocked again since we hit a box that doesn't move, we still want to move all boxes because momentum
				boxesBlocked = true;
			}
		}
		else
		{
			if (IsValid(boxes[0]) && boxes[0]->CanBeFlipped)
			{
				boxes[0]->MoveBoxTo_Internal(boxes[0]->GetBoxCell() + UKBUtility::CardinalToPoint(CardinalOpposite(pushDir)), EBoxMoveType::FLIP);
				break; // Break since we are effectively creating an empty space - no more boxes need to move or be blocked
			}
			else if (boxes[i]->CanBeCrushed && boxes[i]->IsValidForMatches()) // Check valid for matches so we don't crush a box being matched
			{
				boxes[i]->DestroyBox();
				boxesBlocked = false;
			}
		}
	}

	if (boxesBlocked && boxes.Num() > 0)
	{
		boxes[0]->OnBoxCantMove();
		return false;
	}
	else // !boxesBlocked
	{
		return true;
	}
}

void AKBBox::RecursiveBoxMove(ECardinalDirection moveDir, TArray<AKBBox*>& checkedBoxes, AKickboxersCharacter * const pusher /* = nullptr */)
{
	AKBGrid * const grid = GridComponent->GetGrid();
	if (!ensure(grid)) { return; }

	checkedBoxes.Emplace(this);
	Kicker = pusher;
	const FIntPoint targetPoint = GetBoxCell() + UKBUtility::CardinalToPoint(moveDir);

	// Just find the box in the cell so we don't miss falling/moving blocks etc.
	AKBBox * otherBox = grid->GetBoxInCell(targetPoint);

	if (IsValid(otherBox) && ensure(checkedBoxes.Contains(otherBox) == false))
	{
		otherBox->RecursiveBoxMove(moveDir, checkedBoxes, pusher);
	}
	else
	{
		// Empty space - check for players and push them back if we find them
		FCollisionQueryParams collisionQueryParams;
		collisionQueryParams.bIgnoreBlocks = false;
		collisionQueryParams.AddIgnoredActor(this);

		const FVector boxDimension = CardinalToVector(moveDir) * grid->GetCellDimension(IsDirectionHorizontal(moveDir));
		const FVector shapeExtent = BoxCollision->GetScaledBoxExtent() * 0.4f;
		FCollisionShape shape = FCollisionShape::MakeBox(shapeExtent);
		FVector traceLoc = BoxCollision->GetComponentLocation() + boxDimension; // Use this instead of actor loc since the box is bottom-aligned

		const FVector playerTraceshapeExtent = BoxCollision->GetScaledBoxExtent() * 0.8f;
		FCollisionShape playerTraceShape = FCollisionShape::MakeBox(shapeExtent);

		TArray<FHitResult> hits;
		GetWorld()->SweepMultiByChannel(hits, traceLoc, traceLoc, FQuat::Identity, ECollisionChannel::ECC_Pawn, shape, collisionQueryParams);

		for (const auto& hit : hits)
		{
			AKickboxersCharacter * character = Cast<AKickboxersCharacter>(hit.GetActor());
			if (IsValid(character))
			{
				// Use launch character to push the character out of the way (TODO: use this method for the wall push as well?)
				character->PushCharacterAway(moveDir);

				// We moved a character so do another trace for boxes to push behind them
				traceLoc += boxDimension;

				FHitResult boxBehindHit;
				GetWorld()->SweepSingleByChannel(boxBehindHit, traceLoc, traceLoc, FQuat::Identity, COLLISION_BOX, shape, collisionQueryParams);

				AKBBox * boxBehind = Cast<AKBBox>(boxBehindHit.GetActor());
				if (IsValid(boxBehind) && checkedBoxes.Contains(boxBehind) == false)
				{
					boxBehind->RecursiveBoxMove(moveDir, checkedBoxes, pusher);
				}
			}
		}
	}

	if (grid->IsSameRegion(GetBoxCell(), targetPoint))
	{
		MoveBoxTo_Internal(targetPoint, EBoxMoveType::DEFAULT);
	}
	else
	{
		DestroyBox();
	}
}

void AKBBox::OnBoxFinishMove()
{
	AKBGrid * const grid = GridComponent->GetGrid();
	if (!ensure(IsValid(grid)))
	{
		return;
	}

	ensure(HasAuthority());

	bool wasMatched = false;
	// if (Kicker.IsValid()) // Let matches happen regardless of kicker
	{
		// Only check for matches if this box was kicked by a player (i.e. not from wall movement, spawning, etc.)
		wasMatched = grid->OnBoxPlaced(this);
	}

	Kicker.Reset();
	CurrentMoveType = EBoxMoveType::NONE;

	if (!wasMatched)
	{
		grid->CheckStuckCharacters_Server();

		// If a box moved out of bounds, destroy it
		if (grid->IsPointWithinGrid(GetBoxCell()) == false)
		{
			// TODO: Make "fall off world" function/state and appropriate effect
			OnBoxDestroyed_Multicast();
		}
	}
}

void AKBBox::OnBoxMatched_Native()
{
	SetLifeSpan(9999.f); // Kind of hacky, but we use lifespan to tell if a box is valid for matches and this is better than another bool

	DoBoxMatchStartEffects_Multicast();

	// Set timer for box clear
	float matchDelay = OnBoxMatched();
	FTimerHandle BoxMatchHandle;
	GetWorld()->GetTimerManager().SetTimer(BoxMatchHandle, this, &AKBBox::OnMatchTimerFinish_Multicast, matchDelay, false);
}

float AKBBox::OnBoxMatched_Implementation()
{
	return 0.3f; // Default match timer
}

void AKBBox::DoBoxMatchStartEffects_Multicast_Implementation()
{
	OnBoxMatchStart();
}

void AKBBox::OnMatchTimerFinish_Multicast_Implementation()
{
	// Do wall trace on server only
	if (HasAuthority())
	{
		AKBGrid * const grid = GridComponent->GetGrid();
		check(grid);

		FCollisionQueryParams collisionQueryParams;
		collisionQueryParams.bIgnoreBlocks = false;
		collisionQueryParams.AddIgnoredActor(this);

		const FVector boxLoc = BoxCollision->GetComponentLocation();

		TArray<FHitResult> hits;

		float radius = 500.f;
		if (AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>())
		{
			radius = gameState->GameSettings.BoxPushWallRadius;
		}
		FCollisionShape shape = FCollisionShape::MakeSphere(radius);

		// Do explosion and push nearby walls away from the box
		bool didHit = GetWorld()->SweepMultiByChannel(hits, boxLoc, boxLoc, FQuat::Identity, COLLISION_WALL, shape, collisionQueryParams);

		if (didHit)
		{
			const TArray<FGridRegion>& allRegions = grid->GetGridRegions();

			for (auto& hit : hits)
			{
				AKBWall * wall = Cast<AKBWall>(hit.GetActor());
				if (IsValid(wall))
				{
					FCollisionResponseParams responseParams;
					responseParams.CollisionResponse.SetAllChannels(ECollisionResponse::ECR_Ignore);
					responseParams.CollisionResponse.SetResponse(COLLISION_WALL, ECollisionResponse::ECR_Block);

					FHitResult sightCheck;
					const FVector sightVec = boxLoc + (hit.ImpactNormal * -1000.f);
					GetWorld()->LineTraceSingleByChannel(sightCheck, boxLoc, sightVec, COLLISION_WALL, collisionQueryParams, responseParams);

					// Do a sight check as well to make sure we don't hit walls behind this wall that aren't connected to our region
					if (sightCheck.GetActor() == wall)
					{
						ECardinalDirection pushDir;
						if (wall->IsHorizontal())
						{
							pushDir = boxLoc.Y > wall->GetActorLocation().Y ? ECardinalDirection::NORTH : ECardinalDirection::SOUTH;
						}
						else
						{
							pushDir = boxLoc.X > wall->GetActorLocation().X ? ECardinalDirection::WEST : ECardinalDirection::EAST;
						}

						wall->AddPush(pushDir, GetActorLocation());
					}
				}
			}
		}

		// Perform actions on adjacent cells (@TODO - extract and generalize these somehow, this kind of functionality shouldn't be hard coded)
		for (uint8 i = 0; i < TEnumAsByte<ECardinalDirection>(ECardinalDirection::LAST); ++i)
		{
			const ECardinalDirection dir = static_cast<ECardinalDirection>(i);
			const FIntPoint adjacentCell = GetBoxCell() + UKBUtility::CardinalToPoint(dir);

			if (UKBGridCellComponent * gc = FindComponentByClass<UKBGridCellComponent>())
			{
				// Move all adjacent cardinal boxes that are valid for matches (i.e. not these already-matched boxes)
				if (grid->IsSameRegion(gc->GetGridCell(), gc->GetGridCell() + UKBUtility::CardinalToPoint(dir)))
				{
					AKBBox * const adjacentBox = grid->GetBoxInCell(adjacentCell);
					if (IsValid(adjacentBox) && adjacentBox->IsValidForMatches())
					{
						adjacentBox->SetComboNum(GetComboNum());
						adjacentBox->KickBox(dir, GetKicker());
					}
				}
			}

			if (AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>())
			{
				if (gameState->GameSettings.SpawnSproutsOnMatches)
				{
					// Plant new sprouts in all adjacent cardinal cells that don't have a sprout
					AKBSprout * const adjacentSprout = grid->GetSproutInCell(adjacentCell);
					if (!adjacentSprout && !grid->GetBoxInCell(adjacentCell) && grid->IsPointWithinGrid(adjacentCell)) // Check for box again since we just moved boxes
					{
						grid->SpawnRandomSproutInCell(adjacentCell);
					}
				}
			}
		}

		if (GetKicker())
		{
			AKBPlayerState * state = GetKicker()->GetPlayerState<AKBPlayerState>();
			state->OnBoxMatched();
		}
	}

	// Get lifetime from blueprint and let it play effects
	float boxLifetime = OnBoxCleared();

	SetBoxCell(FIntPoint::NoneValue);
	BoxCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxCollision->SetCollisionResponseToAllChannels(ECR_Ignore);

	if (boxLifetime <= 0.f)
	{
		Destroy();
	}
	else
	{
		SetLifeSpan(boxLifetime);
	}
}

void AKBBox::OnBoxDestroyed_Multicast_Implementation()
{
	DestroyBox();
}

void AKBBox::DestroyBox()
{
	float boxLifetime = OnBoxDestroyed();

	SetBoxCell(FIntPoint::NoneValue);

	if (boxLifetime <= 0.f)
	{
		Destroy();
	}
	else
	{
		BoxCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BoxCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
		SetLifeSpan(boxLifetime);
	}
}

void AKBBox::RemoveBox()
{
	Destroy();
}

void AKBBox::SetBoxCell(FIntPoint newCell)
{
	GridComponent->SetGridCell(newCell);

#if !SHIP_OR_TEST
	DebugText->SetText(FText::FromString(FString::Printf(TEXT("%d,%d"), newCell.X, newCell.Y)));
#endif
}

FIntPoint AKBBox::GetBoxCell() const
{
	return GridComponent->GetGridCell();
}

void AKBBox::MoveBoxTo_Internal(FIntPoint cellPoint, EBoxMoveType moveType /* = EBoxMoveType::DEFAULT */)
{
	if (AKBGrid * const grid = GridComponent->GetGrid())
	{
		OnBoxMoved.Broadcast();

		FVector targetLoc = grid->GetCellLocation(cellPoint);
		CurrentMoveType = moveType;
		MoveBoxTo(grid->GetCellLocation(cellPoint), cellPoint, moveType); // Blueprint handles movement

		// Since this function is called on server and all clients, just call this setter on server and let it rep to clients
		if (HasAuthority())
		{
			FinalTargetLocation = grid->GetCellLocation(cellPoint);
			SetBoxCell(cellPoint);
		}
	}
}

bool AKBBox::MatchesType(AKBBox * otherBox) const
{
	return IsValid(otherBox) && otherBox->IsA(GetClass());
}

bool AKBBox::IsValidForMatches() const
{
	// Don't match a box that's already been matched or is destroying
	return (CanBeMatched && !IsPendingKill() && GetLifeSpan() == 0.f && GridComponent->HasValidCell() && IsFinishedSpawning());
}

bool AKBBox::IsFinishedSpawning() const
{
	return CurrentMoveType != EBoxMoveType::SPAWN;
}

void AKBBox::SetBoxEnabled(bool enabled)
{
	SetActorEnableCollision(enabled);
	SetActorTickEnabled(enabled);
	SetActorScale3D(enabled ? FVector::OneVector : FVector(0.66f));
}