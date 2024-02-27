#pragma once

#include "KBDefinitions.generated.h"

// defines go here

USTRUCT()
struct FGridRegion
{
	GENERATED_USTRUCT_BODY()
	
	// UPROPERTYs for replication (USTRUCTs need to have all of their vars marked as UPROPERTY to be properly replicated)
	UPROPERTY()
	FIntPoint StartPoint = FIntPoint::ZeroValue; // Starting grid cell coordinate
	
	UPROPERTY()
	FIntPoint Dimensions = FIntPoint::ZeroValue;

	// Remember to add any newly added properties to the ctors

	bool IsCellWithinRegion(const FIntPoint& cell) const
	{
		return(cell.X >= StartPoint.X && cell.Y >= StartPoint.Y &&
			   cell.X < (StartPoint.X + Dimensions.X) && cell.Y < (StartPoint.Y + Dimensions.Y));
	}

	bool IsZeroRegion() const
	{
		return (StartPoint == FIntPoint::ZeroValue && Dimensions == FIntPoint::ZeroValue);
	}

	// If this region still has cells, i.e. each dimension is > 0
	bool HasCells() const
	{
		return (Dimensions.X > 0 && Dimensions.Y > 0);
	}

	int32 CellCount() const
	{
		return Dimensions.X * Dimensions.Y;
	}

	FGridRegion() { }
	FGridRegion(const FGridRegion& other) 
	{
		StartPoint = other.StartPoint;
		Dimensions = other.Dimensions;
	}

	FGridRegion(FGridRegion *const other)
	{
		StartPoint = other->StartPoint;
		Dimensions = other->Dimensions;
	}

	FGridRegion(FIntPoint start, FIntPoint dimension)
	{
		StartPoint = start;
		Dimensions = dimension;
	}

	inline bool operator==(const FGridRegion& other) const
	{
		return (StartPoint == other.StartPoint
			&& Dimensions == other.Dimensions);
	}

	inline bool operator==(FGridRegion* const other) const
	{
		return (other && StartPoint == other->StartPoint
			&& Dimensions == other->Dimensions);
	}

	inline bool operator!=(FGridRegion& other) const
	{
		return !(*this == other);
	}
};

// Sorts by distance from closest to furthest
struct FSortByDistance
{
	explicit FSortByDistance(const FVector& InSourceLocation)
		: SourceLocation(InSourceLocation)
	{
	}

	/* The Location to use in our Sort comparison. */
	FVector SourceLocation;

	bool operator()(const AActor* A, const AActor* B) const
	{
		float DistanceA = FVector::DistSquared(SourceLocation, A->GetActorLocation());
		float DistanceB = FVector::DistSquared(SourceLocation, B->GetActorLocation());

		return DistanceA < DistanceB;
	}
};

USTRUCT(BlueprintType)
struct FGameSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, EditAnywhere)
	float BoxPushWallRadius = 500.f;

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, EditAnywhere)
	bool SpawnSproutsOnMatches = true;

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, EditAnywhere)
	float SproutSpawnRate = 3.f;

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, EditAnywhere)
	float SproutGrowTime = 0.f;

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, EditAnywhere)
	bool PlayerPickupEnabled = false;

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, EditAnywhere)
	bool PlayerKickEnabled = false;

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, EditAnywhere)
	bool PlayerPickupGrowsSprout = false;

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, EditAnywhere)
	bool AutoSpawnBoxAboveHead = false;

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, EditAnywhere)
	bool PlayerAutoKicksBoxAfterPlacing = true;
};

USTRUCT(BlueprintType)
struct FClientSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, EditAnywhere)
	bool TapToLook = false;

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, EditAnywhere)
	float TapToLookTimer = 0.075f;
};

UENUM(BlueprintType)
enum EPlayerAnimState
{
	IDLE,
	KICKING,
	LIFTING
};

UENUM(BlueprintType)
enum ECardinalDirection
{
	NORTH = 0,
	EAST,
	SOUTH,
	WEST,
	LAST UMETA(Hidden)
};

UENUM(BlueprintType)
enum EBoxMoveType
{
	NONE,
	DEFAULT,
	SPAWN,
	FLIP,
	BOUNCE,
	PUSHED_BY_WALL,
	PLACED,
	GROWN,
	TELEPORT
};

static float CardinalToDegrees(ECardinalDirection dir)
{
	switch (dir)
	{
	case ECardinalDirection::NORTH:
		return 0.f;
	case ECardinalDirection::EAST:
		return 90.f;
	case ECardinalDirection::SOUTH:
		return 180.f;
	case ECardinalDirection::WEST:
		return 270.f;
	default:
		return 0.f;
	}
}

// Clamps given degrees to a 0 to 360 range (-90 becomes 270, etc.)
static float ClampDegrees(float degrees)
{
	return FMath::Fmod(degrees + 360, 360);
}

static ECardinalDirection DegreesToCardinalDir(float degrees)
{
	const float radius = 45.f;

	// Make sure our degrees is 0 to 360 instead of -180 to +180
	degrees = ClampDegrees(degrees);

	for (uint8 i = 0; i < TEnumAsByte<ECardinalDirection>(ECardinalDirection::LAST); ++i)
	{
		ECardinalDirection dir = static_cast<ECardinalDirection>(i);
		if (FMath::Abs(degrees - CardinalToDegrees(dir)) <= radius)
		{
			return dir;
		}
	}

	if (degrees >= 315.f && degrees <= 360.f)
	{
		// Hard coded hack because we can't pick up this range in our above loop
		return ECardinalDirection::NORTH;
	}

	ensureMsgf(false, TEXT("DegreesToCardinalDir - shouldn't be here! degrees was: %f"), degrees);
	return ECardinalDirection::NORTH;
}

static FVector CardinalToVector(ECardinalDirection dir)
{
	switch (dir)
	{
	case ECardinalDirection::NORTH:
		return FVector(0.f, -1.f, 0.f);
	case ECardinalDirection::EAST:
		return FVector(1.f, 0.f, 0.f);
	case ECardinalDirection::SOUTH:
		return FVector(0.f, 1.f, 0.f);
	case ECardinalDirection::WEST:
		return FVector(-1.f, 0.f, 0.f);
	default:
		return FVector::ZeroVector;
	}
}

//static FIntPoint CardinalToPoint(ECardinalDirection dir)
//{
//	switch (dir)
//	{
//	case ECardinalDirection::NORTH:
//		return FIntPoint(0, -1);
//	case ECardinalDirection::EAST:
//		return FIntPoint(1, 0);
//	case ECardinalDirection::SOUTH:
//		return FIntPoint(0, 1);
//	case ECardinalDirection::WEST:
//		return FIntPoint(-1, 0);
//	default:
//		return FIntPoint::ZeroValue;
//	}
//}

//static ECardinalDirection CardinalFromPoint(FIntPoint dir)
//{
//	if (dir == FIntPoint(0, -1))
//	{
//		return ECardinalDirection::NORTH;
//	}
//	else if (dir == FIntPoint(1, 0))
//	{
//		return ECardinalDirection::EAST;
//	}
//	else if (dir == FIntPoint(0, 1))
//	{
//		return ECardinalDirection::SOUTH;
//	}
//	else if (dir == FIntPoint(-1, 0))
//	{
//		return ECardinalDirection::WEST;
//	}
//
//	ensureMsgf(false, TEXT("DegreesToCardinalDir - shouldn't be here! point was: %d,%d"), dir.X, dir.Y);
//	return ECardinalDirection::NORTH;
//}

static ECardinalDirection CardinalOpposite(ECardinalDirection dir)
{
	switch (dir)
	{
	case ECardinalDirection::NORTH:
		return ECardinalDirection::SOUTH;
	case ECardinalDirection::EAST:
		return ECardinalDirection::WEST;
	case ECardinalDirection::SOUTH:
		return ECardinalDirection::NORTH;
	case ECardinalDirection::WEST:
		return ECardinalDirection::EAST;
	default:
		return ECardinalDirection::NORTH;
	}
}

static bool IsDirectionHorizontal(ECardinalDirection dir)
{
	return (dir == ECardinalDirection::EAST || dir == ECardinalDirection::WEST);
}

static const FVector NORTH_DIR = FVector(0.f, -1.f, 0.f); // Our north is -Y due to matching the UE4 X/Y plane with cartesian coords to make grid handling easier

#define COLLISION_WALL ECollisionChannel::ECC_WorldStatic
// Custom collision channels
#define COLLISION_BOX ECollisionChannel::ECC_GameTraceChannel1
#define COLLISION_GRID ECollisionChannel::ECC_GameTraceChannel2
#define COLLISION_INTERACTION ECollisionChannel::ECC_GameTraceChannel3

// Custom trace query channels
#define TRACE_BOX ETraceTypeQuery::TraceTypeQuery1
#define TRACE_GRID ETraceTypeQuery::TraceTypeQuery2

#define BOX_MAX_SPAWN_HEIGHT 500.f
#define SHIP_OR_TEST (UE_BUILD_SHIPPING || UE_BUILD_TEST)

#define BOX_HEIGHT 100.f

// so you need a USTRUCT here with GENERATED_USTRUCT_BODY to generate the generated.h properly (not sure how we got it working on galacide without this)
USTRUCT()
struct FExampleStruct
{
	GENERATED_USTRUCT_BODY()
};
