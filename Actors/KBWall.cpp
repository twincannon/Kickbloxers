// Fill out your copyright notice in the Description page of Project Settings.


#include "KBWall.h"
#include "Kickboxers.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "KickboxersGameMode.h"
#include "Actors/KBGrid.h"
#include "Actors/KBBox.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "KBUtility.h"
#include "KickboxersCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KBGameState.h"

// Sets default values
AKBWall::AKBWall()
{
	WallCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WallCollision"));
	WallCollision->SetBoxExtent(FVector(50.f, 50.f, 100.f));
	WallCollision->SetRelativeLocation(FVector(0.f, 0.f, WallCollision->GetScaledBoxExtent().Z));
	WallCollision->SetCollisionObjectType(COLLISION_WALL);
	WallCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WallCollision->SetCollisionResponseToChannel(COLLISION_GRID, ECollisionResponse::ECR_Overlap);
	WallCollision->ShapeColor = FColor::Yellow;
	WallCollision->BodyInstance.bLockXRotation = true;
	WallCollision->BodyInstance.bLockYRotation = true;
	RootComponent = WallCollision;
	
	PlayerPush = CreateDefaultSubobject<UBoxComponent>(TEXT("PlayerPush"));
	PlayerPush->SetBoxExtent(WallCollision->GetUnscaledBoxExtent());
	PlayerPush->SetCollisionObjectType(COLLISION_WALL);
	PlayerPush->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PlayerPush->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	PlayerPush->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	PlayerPush->SetupAttachment(RootComponent);
	
	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
	WallMesh->SetRelativeScale3D(FVector(1.f, 1.f, 2.f));
	WallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> wallMeshAsset(TEXT("/Game/Geometry/Meshes/1M_Cube.1M_Cube"));
	if (wallMeshAsset.Succeeded())
	{
		WallMesh->SetStaticMesh(wallMeshAsset.Object);
	}
	WallMesh->SetupAttachment(RootComponent);


 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bNetLoadOnClient = false; // We move/scale this actor via the gamemode, and we want those changes to propogate to clients, so we need the clients to not load local copies
								// actually does this even make sense?

}

void AKBWall::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// Need this to happen before AKBGrid::PostInitializeComponents
	if (!IsTemplate())
	{
		TargetLocation = CurrentLocation = GetActorLocation();
	}
}

// Called when the game starts or when spawned
void AKBWall::BeginPlay()
{
	Super::BeginPlay();	

	TargetLocation = CurrentLocation = GetActorLocation();
}

#if WITH_EDITOR
void AKBWall::PostEditChangeProperty(FPropertyChangedEvent & propertyChangedEvent)
{
	Super::PostEditChangeProperty(propertyChangedEvent);

	// Prevent rotation. // TODO: This function doesn't get called when tabbing out of the field?
	if (propertyChangedEvent.MemberProperty && propertyChangedEvent.MemberProperty->GetFName() == "RelativeRotation")
	{
		SetActorRotation(FRotator::ZeroRotator);
	}
}
#endif

void AKBWall::SetWallBaseScale(FVector2D cellSize)
{
	WallCollision->SetBoxExtent(FVector(cellSize.X * 0.5f , cellSize.Y * 0.5f, 100.f));
	PlayerPush->SetBoxExtent(FVector(cellSize.X * 0.5f, cellSize.Y * 0.5f, 100.f));
}

// Called every frame
void AKBWall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		CurrentLocation = GetActorLocation();
		CurrentScale = GetActorScale3D();
	}
	else
	{
		const FVector& actorLoc = GetActorLocation();
		if (!actorLoc.Equals(CurrentLocation))
		{
			const float lerpSmoothness = 0.3f;
			const FVector& location = CurrentLocation;
			SetActorLocation(FMath::Lerp(actorLoc, location, lerpSmoothness));
		}

		const FVector& actorScale = GetActorScale3D();
		if (!actorScale.Equals(CurrentScale))
		{
			const float lerpSmoothness = 0.3f;
			const FVector& scale = CurrentScale;
			SetActorScale3D(FMath::Lerp(actorScale, scale, lerpSmoothness));
		}
	}
}

void AKBWall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKBWall, TargetLocation);
	DOREPLIFETIME(AKBWall, TargetScale);
	DOREPLIFETIME(AKBWall, CurrentLocation);
	DOREPLIFETIME(AKBWall, CurrentScale);
}

bool AKBWall::IsHorizontal() const
{
	FVector scale = GetActorScale3D();
	return GetActorScale3D().X >= GetActorScale3D().Y;
}

void AKBWall::AddPush(ECardinalDirection pushDir, FVector sourceOfPush)
{
	AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>();
	if (!ensure(gameState)) { return; }

	AKBGrid * grid = gameState->GetGrid();
	if (!ensure(grid)) { return; }

	const bool horizontalMove = pushDir == ECardinalDirection::EAST || pushDir == ECardinalDirection::WEST;
	const float cellSize = horizontalMove ? grid->GetCellSize().X : grid->GetCellSize().Y;

	StoredPush += (UKBUtility::CardinalToPoint(pushDir).X + UKBUtility::CardinalToPoint(pushDir).Y);

	SpawnPushParticle_Client(pushDir, sourceOfPush);

	TryApplyPush();
}

void AKBWall::TryApplyPush()
{
	AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>();
	if (!ensure(gameState)) { return; }

	AKBGrid * grid = gameState->GetGrid();
	if (!ensure(grid)) { return; }

	ECardinalDirection pushDir = ECardinalDirection::NORTH;
	bool doMove = false;
	const int32 pushThreshold = 5;
	float cellSize;

	if (IsHorizontal())
	{
		cellSize = grid->GetCellSize().Y;

		// Wall is horizontal so it can be pushed south (positive) or north (negative)
		if (StoredPush >= pushThreshold)
		{
			StoredPush -= pushThreshold;
			pushDir = ECardinalDirection::SOUTH;
			doMove = true;
		}
		else if (StoredPush <= -pushThreshold)
		{
			StoredPush += pushThreshold;
			pushDir = ECardinalDirection::NORTH;
			doMove = true;
		}
	}
	else
	{
		cellSize = grid->GetCellSize().X;

		// Wall is vertical so it can be pushed east (positive) or west (negative)
		if (StoredPush >= pushThreshold)
		{
			StoredPush -= pushThreshold;
			pushDir = ECardinalDirection::EAST;
			doMove = true;
		}
		else if (StoredPush <= -pushThreshold)
		{
			StoredPush += pushThreshold;
			pushDir = ECardinalDirection::WEST;
			doMove = true;
		}
	}

	//TRACEMSG("[%s] TryApplyPush", *ENUM_AS_STRING(ENetRole, GetLocalRole()));

	if (doMove)
	{
		//grid->OnWallMove(this, pushDir); 
		//MoveWallTo_Internal(GetActorLocation() + CardinalToVector(pushDir) * cellSize);

		OnWallRequestMove.ExecuteIfBound(this, pushDir);
		OnWallMoved(pushDir);
	}
	else
	{
		// Pushed, but not moving - do a shake animation
		OnWallPushApplied_Multicast();
	}
}

void AKBWall::SpawnPushParticle_Client_Implementation(ECardinalDirection pushDir, FVector sourceOfPush)
{
	SpawnPushParticle(pushDir, sourceOfPush);
}

void AKBWall::OnWallPushApplied_Multicast_Implementation()
{
	OnWallPushApplied();
}

void AKBWall::MoveWallTo_Internal(FVector newLocation)
{
	//MoveWallTo(newLocation);
}

void AKBWall::UpdateWallLocAndScale(const FIntPoint& nearestCell, const FVector2D& cellSize, const FVector& nearestCellLoc, const FVector& regionStartLoc, const FGridRegion* const region, bool applyImmediately /* = false */)
{
	if (!ensure(region))
	{
		return;
	}

	ensure(HasAuthority());

	//SetActorEnableCollision(false); // Disable collision temporarily just in case
	SetActorRotation(FRotator::ZeroRotator, ETeleportType::ResetPhysics); // Don't support wall rotation - use actor x/y scale instead

	if (IsHorizontal())
	{
		// Wall is horizontal
		const float regionHorizCenter = regionStartLoc.X + region->Dimensions.X * cellSize.X * 0.5f;
		SetWallTargetLocation(FVector(regionHorizCenter, nearestCellLoc.Y - (cellSize.Y * 0.5f), GetActorLocation().Z));
		TargetScale = (FVector(region->Dimensions.X, 0.1f, GetActorScale3D().Z));
	}
	else
	{
		// Wall is vertical
		const float regionVertCenter = regionStartLoc.Y + region->Dimensions.Y * cellSize.Y * 0.5f;
		SetWallTargetLocation(FVector(nearestCellLoc.X - (cellSize.X * 0.5f), regionVertCenter, GetActorLocation().Z));
		TargetScale = (FVector(0.1f, region->Dimensions.Y, GetActorScale3D().Z));
	}

	SetWallBaseScale(cellSize);
	//SetActorEnableCollision(true);

	if (applyImmediately)
	{
		// Apply transforms immediately
		SetActorLocation(TargetLocation);
		SetActorScale3D(TargetScale);
		UpdatePlayerPushScale();

		UpdateWallLocAndScale_Client();
	}
	else
	{
		MoveWallTo(TargetLocation, TargetScale);
	}
}

void AKBWall::UpdateWallLocAndScale_Client_Implementation()
{
	SetActorLocation(TargetLocation);
	SetActorScale3D(TargetScale);
	UpdatePlayerPushScale();
}

void AKBWall::UpdatePlayerPushScale()
{
	FVector pushExtent = WallCollision->GetUnscaledBoxExtent();
	if (TargetScale.X > TargetScale.Y)
	{
		pushExtent.Y *= PlayerPushPadding;
	}
	else
	{
		pushExtent.X *= PlayerPushPadding;
	}

	PlayerPush->SetBoxExtent(pushExtent);
}

void AKBWall::ApplyPlayerPush_Implementation()
{
	if (!IsWallMoving())
	{
		return;
	}

	const FVector wallLoc = GetActorLocation();

	TArray<AActor*> actors;
	PlayerPush->GetOverlappingActors(actors, AKickboxersCharacter::StaticClass());
	for (const auto& actor : actors)
	{
		AKickboxersCharacter * character = Cast<AKickboxersCharacter>(actor);
		if (IsValid(character))
		{
			const FVector charLoc = character->GetActorLocation();
			ECardinalDirection pushDir;

			if (IsHorizontal())
			{
				if (charLoc.Y > wallLoc.Y)
				{
					pushDir = ECardinalDirection::SOUTH;
				}
				else
				{
					pushDir = ECardinalDirection::NORTH;
				}
			}
			else
			{
				if (charLoc.X > wallLoc.X)
				{
					pushDir = ECardinalDirection::EAST;
				}
				else
				{
					pushDir = ECardinalDirection::WEST;
				}
			}

			character->PushCharacterAway(pushDir);
		}
	}
}

TArray<AActor*> AKBWall::GetNeighboringActors(ECardinalDirection direction, const AKBGrid * const grid)
{
	TArray<AActor*> actors;
	// Horizontal walls can only move vertically and vice versa
	const bool correctMove = (IsHorizontal() && !IsDirectionHorizontal(direction) || !IsHorizontal() && IsDirectionHorizontal(direction));
	if (!ensureMsgf(correctMove, TEXT("AKBWall::GetNeighboringActors - Invalid direction specified for this walls alignment")))
	{
		return actors;
	}

	if (!ensure(IsValid(grid))) { return actors; }

	FCollisionQueryParams collisionQueryParams;
	collisionQueryParams.bIgnoreBlocks = false;
	collisionQueryParams.AddIgnoredActor(this);

	TArray<FHitResult> hits;
	FVector shapeExtent = WallCollision->GetScaledBoxExtent();

	float cellSize = grid->GetCellDimension(IsDirectionHorizontal(direction));
	
	if (IsDirectionHorizontal(direction))
	{
		shapeExtent.X = cellSize * 0.4f;
		shapeExtent.Y -= cellSize * 0.5f;
	}
	else
	{
		shapeExtent.Y = cellSize * 0.4f;
		shapeExtent.X -= cellSize * 0.5f;
	}
	
	FCollisionShape shape = FCollisionShape::MakeBox(shapeExtent);


	FVector traceLoc = GetActorLocation();
	traceLoc += CardinalToVector(direction) * cellSize * 0.5f;

	DrawDebugBox(GetWorld(), traceLoc, shapeExtent, FQuat::Identity, FColor::Red, false, 3.0f, 0, 2.f);

	bool didHit = GetWorld()->SweepMultiByChannel(hits, traceLoc, traceLoc, FQuat::Identity, COLLISION_WALL, shape, collisionQueryParams);

	if (didHit)
	{
		for (auto& hit : hits)
		{
			if (IsValid(hit.GetActor()))
			{
				actors.Emplace(hit.GetActor());
			}
		}
	}

	return actors;
}

void AKBWall::WallFinishedMoving()
{
	OnWallFinishedMoving.Broadcast(this);

	if (IsWallAtEdge())
	{
		Destroy();
	}
}