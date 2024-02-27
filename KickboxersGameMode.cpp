// Copyright Epic Games, Inc. All Rights Reserved.

#include "KickboxersGameMode.h"
#include "Kickboxers.h"
#include "KickboxersPlayerController.h"
#include "KickboxersCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Actors/KBGrid.h"
#include "Actors/KBBox.h"
#include "KBGameState.h"
#include "KBPlayerState.h"
#include "KBGameInstance.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/PlayerStartPIE.h"
#include "EngineUtils.h"
#include "KBUserSettings.h"
#include "Pawns/KBSpectatorPawn.h"

AKickboxersGameMode::AKickboxersGameMode()
{
	GameStateClass = AKBGameState::StaticClass();
	PlayerStateClass = AKBPlayerState::StaticClass();
	SpectatorClass = AKBSpectatorPawn::StaticClass();

	bDelayedStart = true;
	MatchReady = false;
	bUseSeamlessTravel = true;
}

void AKickboxersGameMode::BeginPlay()
{
	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		if (UKBGameInstance * gameInstance = GetWorld()->GetGameInstance<UKBGameInstance>())
		{
			if (UKBUserSettings * settings = Cast<UKBUserSettings>(GEngine->GetGameUserSettings()))
			{
				if (settings->OverrideGamemodeSettings)
				{
					// Apply the hosts game settings
					gameState->GameSettings = settings->GameSettings;
				}
			}
		}

		gameState->OnRequiredPlayersReady.AddDynamic(this, &AKickboxersGameMode::DoLongMatchCountdown);
		gameState->OnAllPlayersReady.AddDynamic(this, &AKickboxersGameMode::DoStartMatchCountdown);
		gameState->OnNumPlayersReadyChanged.AddDynamic(this, &AKickboxersGameMode::CheckMatchCountdown);
	}

	Super::BeginPlay();

	// Try to position any manually placed box actors onto the grid and set them up
	AKBGameState * gameState = GetGameState<AKBGameState>();
	if (gameState)
	{
		AKBGrid * grid = gameState->GetGrid();
		if (grid)
		{
			TArray<AActor*> boxActors;
			UGameplayStatics::GetAllActorsOfClass(this, AKBBox::StaticClass(), boxActors);

			for (const auto& actor : boxActors)
			{
				if (AKBBox * box = Cast<AKBBox>(actor))
				{
					const FIntPoint boxCell = grid->GetCellFromWorldLocation(box->GetActorLocation());
					const FVector newBoxLoc = grid->GetCellLocation(boxCell);
					box->SetupBox(grid, boxCell, EBoxMoveType::TELEPORT);
					box->SetOwner(grid);
				}
			}
		}
	}
}

void AKickboxersGameMode::PostLogin(APlayerController * newPlayer)
{
	Super::PostLogin(newPlayer);
}

void AKickboxersGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();
}

void AKickboxersGameMode::NoPlayersAlive()
{
	// No players alive, just end the match
	EndMatch();
}

void AKickboxersGameMode::EndMatch()
{
	Super::EndMatch();

	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		gameState->OnRoundComplete();
	}
}

void AKickboxersGameMode::DoLongMatchCountdown()
{
	// TODO: when numready >= required && numready < required + optional, start a long countdown to start the game with unfilled slots
	DoStartMatchCountdown();
}

void AKickboxersGameMode::DoStartMatchCountdown()
{
	if (GetWorld()->GetTimerManager().IsTimerActive(MatchStartCountdown))
	{
		GetWorld()->GetTimerManager().ClearTimer(MatchStartCountdown);
	}

	GetWorld()->GetTimerManager().SetTimer(MatchStartCountdown, this, &AKickboxersGameMode::SetMatchReady, CountDownLength, false);

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AKickboxersPlayerController* const pc = Cast<AKickboxersPlayerController>(Iterator->Get());
		if (IsValid(pc))
		{
			pc->OnStartCountdown_Client();
		}
	}
}

void AKickboxersGameMode::CheckMatchCountdown(int32 numPlayers, int32 numReady, int32 required, int32 optional)
{
	if (GetWorld()->GetTimerManager().IsTimerActive(MatchStartCountdown))
	{
		if (numReady < required)
		{
			GetWorld()->GetTimerManager().ClearTimer(MatchStartCountdown);

			// We're not ready anymore - cancel the countdown
			for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				AKickboxersPlayerController* const pc = Cast<AKickboxersPlayerController>(Iterator->Get());
				if (IsValid(pc))
				{
					pc->OnCountdownAborted_Client();
				}
			}
		}
		else if (numReady >= required)
		{
			// Ready count changed, restart the timer
			DoStartMatchCountdown();
		}
	}
}

void AKickboxersGameMode::SetPlayerWon(APlayerController * winningPlayer)
{
	if (IsValid(winningPlayer))
	{
		if (AKBPlayerState * kbps = winningPlayer->GetPlayerState<AKBPlayerState>())
		{
			kbps->SetIsWinner(true);
			EndMatch();
		}
	}
}

void AKickboxersGameMode::SetMatchReady()
{
	OnMatchSetReady.Broadcast();

	MatchReady = true;
}

bool AKickboxersGameMode::ReadyToStartMatch_Implementation()
{

	// Also maybe just return super:: here and override this in base classes... i.e. KBGameModeVersus, KBGameModePuzzle, etc <-- do this
	return MatchReady;
}

void AKickboxersGameMode::StartMatch()
{
	Super::StartMatch();

	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		// Let gamestate know the match started
		gameState->OnMatchStarted_Multicast();

		// By default, for all our game modes, handle no players being alive
		gameState->OnNoPlayersAlive.AddDynamic(this, &AKickboxersGameMode::NoPlayersAlive);
	}

	StartBoxTimer();
}

void AKickboxersGameMode::StartBoxTimer()
{
	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		if (gameState->GameSettings.SproutSpawnRate > 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(BoxSpawnTimer, this, &AKickboxersGameMode::SpawnBox, gameState->GameSettings.SproutSpawnRate, true);
		}
	}

	// TODO: Make this spawn veggie patches
}

void AKickboxersGameMode::SpawnBox()
{
	AKBGameState * gameState = GetGameState<AKBGameState>();
	if (gameState && !gameState->GetGrid())
	{
		GetWorld()->GetTimerManager().ClearTimer(BoxSpawnTimer);
		return;
	}

	if (SproutClasses.Num() > 0)
	{
		gameState->GetGrid()->SpawnSproutInEachRegion(SproutClasses[FMath::RandRange(0, SproutClasses.Num() - 1)]);
	}
	
	//if (BoxClasses.Num() > 0)
	//{
		//gameState->GetGrid()->SpawnRandomBoxInEachRegion(BoxClasses[FMath::RandRange(0, BoxClasses.Num() - 1)]);

		// consider moving some of this to gamestate where grid is now, or use delegates instead of all this grossness

		// TODO: also make it spawn faster at the start for like 5 boxes, maybe? just to fill grid? (rather than have dangling)
		// or just spawn a few skipping animation on beginplay - yea this is better

		// TODO: use an array to keep track of spawns per region.. so each region gets the same pattern (types of blocks, not locations)
	//}
}

void AKickboxersGameMode::KB_SetBoxSpawningEnabled(bool enabled)
{
	if (enabled	&& GetWorld()->GetTimerManager().IsTimerActive(BoxSpawnTimer) == false)
	{
		GetWorld()->GetTimerManager().SetTimer(BoxSpawnTimer, this, &AKickboxersGameMode::SpawnBox, 3.f, true);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(BoxSpawnTimer);
	}

	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		gameState->SendPlayerMsg(FString::Printf(TEXT("BoxSpawningEnabled is now %s"), enabled ? "true" : "false"));
	}
}

void AKickboxersGameMode::KB_SpawnBox(int32 x, int32 y)
{
	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		if (IsValid(gameState->GetGrid()) && BoxClasses.Num() > 0)
		{
			gameState->GetGrid()->SpawnBoxInCell(FIntPoint(x, y), BoxClasses[0], EBoxMoveType::DEFAULT);
		}
	}
}

void AKickboxersGameMode::KB_SetBoxSpawnRate(float rate)
{
	if (GetWorld()->GetTimerManager().IsTimerActive(BoxSpawnTimer))
	{
		GetWorld()->GetTimerManager().ClearTimer(BoxSpawnTimer);
	}

	GetWorld()->GetTimerManager().SetTimer(BoxSpawnTimer, this, &ThisClass::SpawnBox, rate, true);

	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		gameState->SendPlayerMsg(FString::Printf(TEXT("BoxSpawnTimer is now %s"), *FString::SanitizeFloat(rate)));
	}
}

void AKickboxersGameMode::KB_SetBoxWallPushRadius(float radius)
{
	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		gameState->GameSettings.BoxPushWallRadius = radius;
		gameState->SendPlayerMsg(FString::Printf(TEXT("BoxPushWallRadius is now %s"), *FString::SanitizeFloat(radius)));
	}
}

void AKickboxersGameMode::KB_SetSproutGrowTime(float time)
{
	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		gameState->OverrideSproutGrowTimer = true;
		gameState->GameSettings.SproutGrowTime = time;
		gameState->SendPlayerMsg(FString::Printf(TEXT("SproutGrowTime is now %s"), *FString::SanitizeFloat(time)));
	}
}

void AKickboxersGameMode::KB_SetPlayerPickupEnabled(bool enabled)
{
	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		gameState->GameSettings.PlayerPickupEnabled = enabled;
		gameState->SendPlayerMsg(enabled ? FString::Printf(TEXT("PlayerPickupEnabled is now enabled")) :
										   FString::Printf(TEXT("PlayerPickupEnabled is now disabled")));
	}
}

void AKickboxersGameMode::KB_SetPlayerKickEnabled(bool enabled)
{
	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		gameState->GameSettings.PlayerKickEnabled = enabled;
		gameState->SendPlayerMsg(enabled ? FString::Printf(TEXT("PlayerKickEnabled is now enabled")) :
										   FString::Printf(TEXT("PlayerKickEnabled is now disabled")));
	}
}

void AKickboxersGameMode::KB_SetPlayerPickupGrowsSprout(bool enabled)
{
	if (AKBGameState * gameState = GetGameState<AKBGameState>())
	{
		gameState->GameSettings.PlayerPickupGrowsSprout = enabled;
		gameState->SendPlayerMsg(enabled ? FString::Printf(TEXT("PlayerPickupGrowsSprout is now enabled")) :
										   FString::Printf(TEXT("PlayerPickupGrowsSprout is now disabled")));
	}
}

TSubclassOf<AKBBox> AKickboxersGameMode::GetRandomBoxClass() const
{
	return BoxClasses.Num() > 0 ? BoxClasses[FMath::RandRange(0, BoxClasses.Num() - 1)] : nullptr;
}

TSubclassOf<AKBSprout> AKickboxersGameMode::GetRandomSproutClass() const
{
	return SproutClasses.Num() > 0 ? SproutClasses[FMath::RandRange(0, SproutClasses.Num() - 1)] : nullptr;
}