// Copyright Puny Human LLC 2020


#include "KBSprout.h"
#include "KBGrid.h"
#include "KBBox.h"
#include "KickboxersGameMode.h"
#include "Kickboxers.h"
#include "Net/UnrealNetwork.h"
#include "KBGameState.h"
#include "Components/KBGridCellComponent.h"
#include "KBUtility.h"

// Sets default values
AKBSprout::AKBSprout()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GridComponent = CreateDefaultSubobject<UKBGridCellComponent>(TEXT("GridComponent"));

	bReplicates = true;
}

// Called when the game starts or when spawned
void AKBSprout::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GridComponent->OnPlayerLeftCell.AddDynamic(this, &AKBSprout::ManualGrowSprout);

		// Check box class validity
		if (!GetBoxToSpawnWhenGrown())
		{
			// Fallback to random class from gamemode
			AKickboxersGameMode * gameMode = Cast<AKickboxersGameMode>(GetWorld()->GetAuthGameMode());
			BoxToSpawnWhenGrown = gameMode->GetRandomBoxClass();
		}

		if (!GetBoxToSpawnWhenGrown())
		{
			ERROR("AKBSprout has no valid box class, aborting beginplay");
			return;
		}

		if (AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>())
		{
			float growTime = gameState->GameSettings.SproutGrowTime;

			if (growTime >= 0.f)
			{
				GetWorld()->GetTimerManager().SetTimer(GrowHandle, this, &AKBSprout::TryToGrow, growTime, false);

				if (growTime > 1.f)
				{
					GetWorld()->GetTimerManager().SetTimer(WarningHandle, this, &AKBSprout::OnGrowWarning_Multicast, growTime - 1.f, false);
				}
			}

			if (SproutTime >= 0.f && (SproutTime < growTime || growTime <= 0.f))
			{
				GetWorld()->GetTimerManager().SetTimer(SproutHandle, this, &AKBSprout::OnSprouted_Multicast, SproutTime, false);
			}
		}
	}
}

void AKBSprout::ManualGrowSprout()
{
	OnGrowWarning_Multicast();
	TryToGrow();
}

void AKBSprout::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKBSprout, BoxToSpawnWhenGrown);
}

UStaticMesh * AKBSprout::GetBoxMesh() const
{
	if (GetBoxToSpawnWhenGrown())
	{
		return UKBUtility::GetBoxMeshFromClass(GetBoxToSpawnWhenGrown());
	}

	return nullptr;
}

void AKBSprout::TryToGrow()
{
	GetWorld()->GetTimerManager().ClearTimer(GrowHandle);

	AKBGrid * grid = Cast<AKBGrid>(GetOwner());
	if (grid && GridComponent->IsCellClear())
	{
		grid->SpawnBoxInCell(GridComponent->GetGridCell(), GetBoxToSpawnWhenGrown(), EBoxMoveType::GROWN);

		OnGrown_Multicast(); // Allow for bloom animation etc.

		if (Regrows)
		{
			if (AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>())
			{
				GetWorld()->GetTimerManager().SetTimer(GrowHandle, this, &AKBSprout::TryToGrow, gameState->GameSettings.SproutGrowTime, false);
			}
		}
		else
		{
			GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

			// If lifespan wasn't set, destroy immediately
			if (GetLifeSpan() == 0.0f)
			{
				Destroy();
			}
		}
	}
	else
	{
		// Failed to grow, try again soon
		const float retryGrowTime = 1.f;
		GetWorld()->GetTimerManager().SetTimer(GrowHandle, this, &AKBSprout::TryToGrow, retryGrowTime, false);
	}
}

void AKBSprout::OnSprouted_Multicast_Implementation()
{
	OnSprouted();
}

void AKBSprout::OnGrowWarning_Multicast_Implementation()
{
	OnGrowWarning();
}

void AKBSprout::OnGrown_Multicast_Implementation()
{
	OnGrown();
}

void AKBSprout::OnWatered_Internal()
{
	OnWatered();
	TryToGrow();
}

void AKBSprout::OnPicked_Multicast_Implementation()
{
	OnPicked();

	if (HasAuthority())
	{
		// If lifespan wasn't set, destroy immediately
		if (GetLifeSpan() == 0.0f)
		{
			Destroy();
		}
	}
}

// Called every frame
void AKBSprout::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

