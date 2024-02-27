// Copyright Epic Games, Inc. All Rights Reserved.

#include "KickboxersPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "KickboxersCharacter.h"
#include "Engine/World.h"
#include "Actors/KBBox.h"
#include "Kickboxers.h"
#include "Net/UnrealNetwork.h"
#include "KBPlayerState.h"
#include "KBGameState.h"
#include "Actors/KBGrid.h"
#include "KickboxersGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "KBHUD.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Actors/KBPlayerStart.h"
#include "Actors/KBCamera.h"
#include "KBUtility.h"
#include "Components/KBGridCellComponent.h"
#include "Components/KBPlayerInteractComponent.h"
#include "KBUserSettings.h"

AKickboxersPlayerController::AKickboxersPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;

	bAutoManageActiveCameraTarget = false;
	StateName = NAME_Spectating;
	bPlayerIsWaiting = true;
}

void AKickboxersPlayerController::BeginPlay()
{
	Super::BeginPlay();

	HideLoadingScreen();

	if (IsLocalController())
	{
		FindViewTarget();
	}

	// Note: BeginPlay doesn't get called for controllers after seamless travel, use PostSeamlessTravel()
}

void AKickboxersPlayerController::OnLoginToLobby()
{
	StateName = NAME_Playing;
	bPlayerIsWaiting = false;
}

void AKickboxersPlayerController::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	HideLoadingScreen();

	if (IsLocalController())
	{
		FindViewTarget();
	}
}

void AKickboxersPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
}

void AKickboxersPlayerController::FindViewTarget()
{
	AActor * viewTarget = nullptr;

	// Location should be pawn starting location, but we don't have a pawn on game start, default to 0,0,0 for spectating
	FVector sourceLoc = IsValid(GetPawn()) ? GetPawn()->GetActorLocation() : FVector::ZeroVector;

	// Find closest player start, see if it's a KBPlayerStart and has a specific camera to use
	TArray<AActor*> startActors;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), startActors);
	if (startActors.Num() > 0)
	{
		Algo::Sort(startActors, FSortByDistance(sourceLoc));

		if (AKBPlayerStart * kbStart = Cast<AKBPlayerStart>(startActors[0]))
		{
			if (kbStart->CameraToUse)
			{
				viewTarget = kbStart->CameraToUse;
			}
		}
	}

	// Fallback to finding the closest camera
	if (!viewTarget)
	{
		TArray<AActor*> cameraActors;
		UGameplayStatics::GetAllActorsOfClass(this, ACameraActor::StaticClass(), cameraActors);
		Algo::Sort(cameraActors, FSortByDistance(sourceLoc));

		for (const auto& camera : cameraActors)
		{
			// Stupid hack to ignore cameras at 0,0,0 because PlayerCameraManager spawns a camera actor for no good reason
			if (IsValid(camera) && camera->GetActorLocation() != FVector::ZeroVector)
			{
				viewTarget = camera;
				break;
			}
		}
	}

	if (viewTarget)
	{
		SetViewTarget(viewTarget);

		if (AKBCamera * kbCamera = Cast<AKBCamera>(viewTarget))
		{
			MoveAngleOffset = kbCamera->AngleOffset;
		}
	}
}

void AKickboxersPlayerController::AcknowledgePossession(class APawn* p)
{
	Super::AcknowledgePossession(p);

	FindViewTarget();
}

void AKickboxersPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// We start waiting when we ready up
	bPlayerIsWaiting = false;
	StateName = NAME_Spectating;
}

void AKickboxersPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	ShowLoadingScreen();

	// Possible TODO: if HUD persists through transition map, destroy it here?
	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);
}

void AKickboxersPlayerController::InitPlayerState()
{
	// Server
	Super::InitPlayerState();

	if (!IsTemplate() && !HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		if (GetNetMode() != NM_Client)
		{
			AKBPlayerState * state = GetPlayerState<AKBPlayerState>();
			if (ensure(state))
			{
				state->OnPlayerReadyStateChange.AddDynamic(this, &AKickboxersPlayerController::OnReadyStateChanged);
			}
		}
	}
}

void AKickboxersPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdateGridMovement();
}

void AKickboxersPlayerController::UpdateGridMovement()
{
	// WARNING THIS IS AWFUL - if we're grid moving, move towards destination target
	bool auth = (!HasAuthority() && GetLocalRole() == ENetRole::ROLE_AutonomousProxy) || (IsLocalController() && HasAuthority());
	if (auth && MovingOnGrid && GetPawn())
	{
		const FVector currentLoc = GetPawn()->GetActorLocation();

		switch (CurrentGridMoveDir)
		{
		case ECardinalDirection::NORTH:
			MovePawnBasedOnViewTarget(1.0f);
			break;
		case ECardinalDirection::SOUTH:
			MovePawnBasedOnViewTarget(-1.0f);
			break;
		case ECardinalDirection::EAST:
			MovePawnBasedOnViewTarget(1.0f, 90.f);
			break;
		case ECardinalDirection::WEST:
			MovePawnBasedOnViewTarget(-1.0f, 90.f);
			break;
		default:
			break;
		}

		if (AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>())
		{
			if (AKBGrid * grid = gameState->GetGrid())
			{
				// Check if we passed any threshold
				const float buffer = 15.f; // dumb

				const FVector2D prev = FVector2D(FMath::Abs(PreviousGridLoc.X), FMath::Abs(PreviousGridLoc.Y));
				const FVector2D cur = FVector2D(FMath::Abs(currentLoc.X), FMath::Abs(currentLoc.Y));
				const FVector2D target = FVector2D(FMath::Abs(TargetGridLocation.X), FMath::Abs(TargetGridLocation.Y));

				if (prev.X < target.X - buffer && cur.X >= target.X - buffer ||
					prev.Y < target.Y - buffer && cur.Y >= target.Y - buffer ||
					prev.X > target.X + buffer && cur.X <= target.X + buffer ||
					prev.Y > target.Y + buffer && cur.Y <= target.Y + buffer)
				{
					MovingOnGrid = false;
				}
				else if (PrevMovingOnGrid && FVector::Dist(currentLoc, PreviousGridLoc) < 0.1f)
				{
					// We didn't move much - we must be stuck or something, abort the movement
					MovingOnGrid = false;
				}

				if (!MovingOnGrid && QueuedGridMove)
				{
					// Stopped moving but we have a queued loc - do it
					//TargetGridLocation = QueuedGridLoc;
					FIntPoint queuedCell = grid->GetCellFromWorldLocation(QueuedGridLoc);
					//MovingOnGrid = true;
					QueuedGridMove = false;

					// Figure out our move dir since we couldn't set it before
					//CurrentGridMoveDir = CardinalFromPoint(queuedCell - TargetGridCell);

					//TargetGridCell = queuedCell;
						
					StartGridMoveInDirection(UKBUtility::CardinalFromPoint(queuedCell - GetCharacterTargetGridCell()));
				}
			}
		}

		if (!MovingOnGrid)
		{
			// Buffer time - if we just stopped grid moving, pressing a direction within this time will skip tap-to-turn
			const float bufferTime = 0.033f;
			GetWorld()->GetTimerManager().SetTimer(GridMoveBufferHandle, bufferTime, false);

		}

		PreviousGridLoc = currentLoc;
	}

	PrevMovingOnGrid = MovingOnGrid;
}

FIntPoint AKickboxersPlayerController::GetCharacterTargetGridCell() const
{
	if (UKBGridCellComponent * gridComp = GetPawn()->FindComponentByClass<UKBGridCellComponent>())
	{
		return gridComp->GetGridCell();
	}

	return FIntPoint::NoneValue;
}

void AKickboxersPlayerController::SetCharacterTargetGridCell(FIntPoint newTargetCell)
{
	if (UKBGridCellComponent * gridComp = GetPawn()->FindComponentByClass<UKBGridCellComponent>())
	{
		const FIntPoint oldCell = gridComp->GetGridCell();
		gridComp->SetGridCell(newTargetCell);

		if (HasAuthority())
		{
			gridComp->GetGrid()->NotifyGridComponentsOfPlayerMove(oldCell);
		}
	}
}

void AKickboxersPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAxis("MoveNorth", this, &AKickboxersPlayerController::OnInputNorth);
	InputComponent->BindAxis("MoveEast", this, &AKickboxersPlayerController::OnInputEast);
	InputComponent->BindAction("Kick", IE_Pressed, this, &AKickboxersPlayerController::OnInputKick);
	InputComponent->BindAction("Pickup", IE_Pressed, this, &AKickboxersPlayerController::OnInputPickup);
	InputComponent->BindAction("Ready", IE_Pressed, this, &AKickboxersPlayerController::OnInputReady);
	InputComponent->BindAction("Action", IE_Pressed, this, &AKickboxersPlayerController::OnInputAction);

	InputComponent->BindAction("PickupNorth", IE_Pressed, this, &AKickboxersPlayerController::PickupInDirection<ECardinalDirection::NORTH>);
	InputComponent->BindAction("PickupEast", IE_Pressed, this, &AKickboxersPlayerController::PickupInDirection<ECardinalDirection::EAST>);
	InputComponent->BindAction("PickupSouth", IE_Pressed, this, &AKickboxersPlayerController::PickupInDirection<ECardinalDirection::SOUTH>);
	InputComponent->BindAction("PickupWest", IE_Pressed, this, &AKickboxersPlayerController::PickupInDirection<ECardinalDirection::WEST>);
}

void AKickboxersPlayerController::OnInputNorth(float axisAmount)
{
	// 45 degrees is +Y north
	// -45 degrees Is +X north (315)
	// -135 is -Y north (225)
	// 135 is -X north
	//const float viewYaw = (GetViewTarget()->GetActorRotation().Yaw + 360) % 360; // Clamp degrees 0 to 360

	const float axisDeadzone = 0.1f;
	bool anyInput = (axisAmount > axisDeadzone || axisAmount < -axisDeadzone);
	if (anyInput)
	{
		bool didGridMove = false;
		if (axisAmount > axisDeadzone)
		{
			didGridMove = StartGridMoveInDirection(ECardinalDirection::NORTH);
		}
		else if (axisAmount < -axisDeadzone)
		{
			didGridMove = StartGridMoveInDirection(ECardinalDirection::SOUTH);
		}

		if (!didGridMove)
		{
			// Fall back to regular movement
			MovePawnBasedOnViewTarget(axisAmount);
		}
	}

	PreviousMoveKeyHeld = anyInput;
}

void AKickboxersPlayerController::OnInputEast(float axisAmount)
{
	const float axisDeadzone = 0.1f;
	bool anyInput = (axisAmount > axisDeadzone || axisAmount < -axisDeadzone);
	if (anyInput)
	{
		bool didGridMove = false;
		if (axisAmount > axisDeadzone)
		{
			didGridMove = StartGridMoveInDirection(ECardinalDirection::EAST);
		}
		else if (axisAmount < -axisDeadzone)
		{

			didGridMove = StartGridMoveInDirection(ECardinalDirection::WEST);
		}

		if (!didGridMove)
		{
			// Fall back to regular movement
			MovePawnBasedOnViewTarget(axisAmount, 90.f); // adding 90 since this is the right vector
		}
	}

	PreviousMoveKeyHeld = anyInput;
}


#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

void AKickboxersPlayerController::OnInputAction()
{
	DoActionTrace_Server();
}

void AKickboxersPlayerController::DoActionTrace()
{
	if (AKickboxersCharacter * character = GetPawn<AKickboxersCharacter>())
	{
		if (AKBGrid * grid = character->GridComponent->GetGrid())
		{
			const FIntPoint playerCell = grid->GetCellFromWorldLocation(character->GetActorLocation());
			const FIntPoint targetCell = playerCell + UKBUtility::CardinalToPoint(DegreesToCardinalDir(character->GetActorRotation().Yaw + 90.f));

			FCollisionQueryParams collisionQueryParams;
			collisionQueryParams.bIgnoreBlocks = false;

			FVector shapeExtent = FVector(30.f);
			FCollisionShape shape = FCollisionShape::MakeBox(shapeExtent);

			FVector traceLoc = grid->GetCellLocation(targetCell);
			traceLoc.Z += 50.f;

			DrawDebugLine(GetWorld(), character->GetActorLocation(), traceLoc, FColor::Red, false, 10.f);
			DrawDebugBox(GetWorld(), traceLoc, shapeExtent, FQuat::Identity, FColor::Red, false, 3.0f, 0, 2.f);

			FCollisionResponseParams responseParams;
			responseParams.CollisionResponse.SetAllChannels(ECollisionResponse::ECR_Ignore);
			responseParams.CollisionResponse.SetResponse(COLLISION_INTERACTION, ECollisionResponse::ECR_Block);

			FHitResult hit;
			if (GetWorld()->LineTraceSingleByChannel(hit, character->GetActorLocation(), traceLoc, COLLISION_INTERACTION, collisionQueryParams, responseParams))
			{
				if (hit.Actor.IsValid())
				{
					if (UKBPlayerInteractComponent * component = hit.Actor->FindComponentByClass<UKBPlayerInteractComponent>())
					{
						component->OnPlayerInteract.Broadcast(character);
					}
				}
			}
		}
	}
}

void AKickboxersPlayerController::DoActionTrace_Server_Implementation()
{
	DoActionTrace();
}

bool AKickboxersPlayerController::StartGridMoveInDirection(ECardinalDirection dir, bool allowOutOfBounds /* = false */)
{
	AKickboxersCharacter * character = GetPawn<AKickboxersCharacter>();
	AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>();

	if (IsValid(character) && gameState)
	{
		if (AKBGrid * grid = character->GridComponent->GetGrid())
		{
			if (grid->IsLocationWithinGrid(character->GetActorLocation()))
			{
				if (!MovingOnGrid)
				{
					ECardinalDirection prevFacing = character->GetFacingDirection(true);

					// Turn to look in the direction
					float degrees = CardinalToDegrees(dir);
					degrees += GetViewTarget()->GetActorRotation().Yaw;
					character->SetCharacterRotation(FRotator(0.f, degrees, 0.f));

					FTimerManager& timerManager = GetWorld()->GetTimerManager();
					ECardinalDirection currentFacing = character->GetFacingDirection(true);

					UKBUserSettings * const settings = Cast<UKBUserSettings>(GEngine->GetGameUserSettings());

					// If a move key was held the previous frame, don't do tap-to-turn (assume the player wants to continue moving)
					if (settings && settings->ClientSettings.TapToLook && currentFacing != prevFacing && !PreviousMoveKeyHeld && !timerManager.IsTimerActive(GridMoveBufferHandle))
					{
						// Turn to face dir and prevent movement for a short time (so player can tap to turn)
						timerManager.SetTimer(GridTurnHandle, settings->ClientSettings.TapToLookTimer, false);
					}
					else if (!timerManager.IsTimerActive(GridTurnHandle))
					{
						// Do grid movement
						CurrentGridMoveDir = dir;

						// Get adjusted degrees based on our viewtarget rotation (camera)
						float adjustedDegrees = CardinalToDegrees(CurrentGridMoveDir);
						adjustedDegrees += GetViewTarget()->GetActorRotation().Yaw + 90.f;
						ECardinalDirection adjustedDir = DegreesToCardinalDir(adjustedDegrees);

						// Get player/target cells
						const FIntPoint playerCell = grid->GetCellFromWorldLocation(character->GetActorLocation());
						const FIntPoint targetCell = playerCell + UKBUtility::CardinalToPoint(adjustedDir);
						
						if (grid->IsPointWithinGrid(targetCell) || allowOutOfBounds) // Clients know about grid dimensions so this is OK to call (but they don't know about region data)
						{
							AKBBox * box = grid->GetBoxInCell(targetCell);
							AActor * blockingActor = nullptr;
							if (grid->IsCellClear(targetCell, { }, blockingActor) || (IsValid(box) && box->CanBeMoved())) // @TODO Naive - assumes that if a box is in a cell then it is otherwise OK to move into.
							{
								MovingOnGrid = true;

								if (!HasAuthority()) // Kind of hacky - but for listenserver we don't want to call this twice (it gets called inside OnPlayerGridMoved_Server)
								{
									TargetGridLocation = grid->GetCellLocation(targetCell);
									SetCharacterTargetGridCell(targetCell);
								}

								// Tell server about our move so it can affect the grid state and double check us and such
								OnPlayerGridMoved_Server(adjustedDir, allowOutOfBounds);
							}
							else
							{
								OnPlayerBumpedIntoObject_Server(blockingActor);
							}
						}
						else if (timerManager.IsTimerActive(PlayerFailedMoveTimer) == false)
						{
							timerManager.SetTimer(PlayerFailedMoveTimer, 0.4f, false); // Set a timer so it doesn't spam RPCs
							OnPlayerTriedMovingOutsideGrid_Server(grid, targetCell);
						}
					}
				}

				// If we're within grid, we are using grid move, whether or not the above logic is relevant
				return true;
			}
		}
	}

	return false;
}

void AKickboxersPlayerController::OnPlayerBumpedIntoObject(AActor * blockingActor)
{
	if (AKickboxersCharacter * character = GetPawn<AKickboxersCharacter>())
	{
		if (IsValid(blockingActor))
		{
			if (UKBPlayerInteractComponent * component = blockingActor->FindComponentByClass<UKBPlayerInteractComponent>())
			{
				component->OnPlayerBumpedInto.Broadcast(character);
			}
		}
	}

	// Also try and activate whatever we bumped into
	DoActionTrace();
}

void AKickboxersPlayerController::OnPlayerBumpedIntoObject_Server_Implementation(AActor * blockingActor)
{
	OnPlayerBumpedIntoObject(blockingActor);
}

void AKickboxersPlayerController::OnPlayerTriedMovingOutsideGrid_Server_Implementation(AKBGrid * grid, FIntPoint targetCell)
{
	AKickboxersCharacter * character = GetPawn<AKickboxersCharacter>();
	if (grid && character)
	{
		grid->OnPlayerTriedMovingOutsideGrid.Broadcast(character, targetCell);
	}
}

void AKickboxersPlayerController::OnPlayerGridMoved_Server_Implementation(ECardinalDirection dir, bool allowOutOfBounds)
{
	if (AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>())
	{
		if (AKBGrid * grid = gameState->GetGrid())
		{
			if (AKickboxersCharacter * character = GetPawn<AKickboxersCharacter>())
			{
				// Get player/target cells (do it again on server as a kind of validation, but use the client's given direction)
				const FVector playerLoc = character->GetActorLocation();
				const FIntPoint playerCell = grid->GetCellFromWorldLocation(playerLoc);
				const FIntPoint targetCell = playerCell + UKBUtility::CardinalToPoint(dir);
				const FVector targetLoc = grid->GetCellLocation(targetCell);
				const bool sameRegion = grid->IsSameRegion(playerCell, targetCell);

				AKBBox * box = grid->GetBoxInCell(targetCell);

				bool didMove = false;

				AActor * blockingActor = nullptr;

				if (grid->IsPointWithinGrid(targetCell))
				{
					if (sameRegion && IsValid(box))
					{
						// Push the box (given there is a box there it should also be a valid place for us to stand - still an assumption though)
						didMove = box->PushBoxRow(dir, character); // Push all boxes a single tile

						TargetGridLocation = targetLoc;
						SetCharacterTargetGridCell(targetCell);

					}
					else if (grid->IsCellClear(targetCell, { GetPawn() }, blockingActor) && (sameRegion /*grid->IsPointWithinGrid(targetCell)*/ || allowOutOfBounds))
					{
						didMove = true;

						// Cell is clear and valid - begin moving there
						TargetGridLocation = targetLoc;
						SetCharacterTargetGridCell(targetCell);
					}
				}

				if (!didMove)
				{
					// Cell isn't clear - abort the movement
					//MovingOnGrid = false;

					// Since the client thought it moved, we want to execute this logic here
					OnPlayerBumpedIntoObject(blockingActor);

					// Tell the client that the move failed
					CorrectGridMove_Client(playerCell, playerLoc);

					// We still want the player to face the direction though (obsoleted?)
					//FRotator facingRot = FRotator(0.f, adjustedDegrees - 90.f, 0.f);
					//character->SetCharacterRotation(facingRot);
				}
			}
		}
	}
}

void AKickboxersPlayerController::CorrectGridMove_Client_Implementation(FIntPoint playerCell, FVector targetLoc)
{
	// @TODO - this doesn't work? (the intent is that if the client moves somewhere and the server disagrees
	// this corrects them and they backpedal back to the previous cell)

	TargetGridLocation = targetLoc;
	SetCharacterTargetGridCell(playerCell);
}

FVector AKickboxersPlayerController::MovePawnBasedOnViewTarget(float axisAmount, float additionalAngle /* = 0.f */)
{
	if (GetViewTarget() && GetPawn() && FMath::Abs(axisAmount) >= 0.3f)
	{
		GetPawn()->ConsumeMovementInputVector();

		FRotator viewRot = GetViewTarget()->GetActorRotation();
		viewRot.Pitch = 0.f;
		viewRot.Roll = 0.f;
		const float CAMERA_ANGLE_OFFSET = MoveAngleOffset + additionalAngle;
		viewRot.Yaw += CAMERA_ANGLE_OFFSET;
		const FVector moveDir = viewRot.Vector() * axisAmount;
		const float tol = KINDA_SMALL_NUMBER;
	
		FVector pending = GetPawn()->GetPendingMovementInputVector();
		//if (moveDir.X != 0.f && pending.X == 0.f || moveDir.Y != 0.f && pending.Y == 0.f)
		if (!FMath::IsNearlyZero(moveDir.X, tol) && FMath::IsNearlyZero(pending.X, tol) ||
			!FMath::IsNearlyZero(moveDir.Y, tol) && FMath::IsNearlyZero(pending.Y, tol))
		{
			GetPawn()->AddMovementInput(moveDir);

			return moveDir;
		}
	}

	return FVector::ZeroVector;
}

void AKickboxersPlayerController::OnInputKick()
{
	bool canKick = true;
	if (AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>())
	{
		canKick = gameState->GameSettings.PlayerKickEnabled;
	}

	if (canKick)
	{
		AKickboxersCharacter * pawn = Cast<AKickboxersCharacter>(GetPawn());
		if (IsValid(pawn))
		{
			pawn->OnKick();
		}
	}
}

void AKickboxersPlayerController::OnInputPickup()
{
	bool canPickup = true;
	if (AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>())
	{
		canPickup = gameState->GameSettings.PlayerPickupEnabled;
	}

	if (canPickup)
	{
		AKickboxersCharacter * pawn = Cast<AKickboxersCharacter>(GetPawn());
		if (IsValid(pawn))
		{
			//if (pawn->GetLiftedBox() == nullptr)
			{
				pawn->OnPickup_Server();
			}
			//else
			//{
			//	pawn->TryPlaceLiftedBox();
			//}
		}
	}
}

template<ECardinalDirection pickupDir>
void AKickboxersPlayerController::PickupInDirection()
{
	if (AKickboxersCharacter * character = GetPawn<AKickboxersCharacter>())
	{
		float degrees = CardinalToDegrees(pickupDir);
		degrees += GetViewTarget()->GetActorRotation().Yaw;
		character->SetCharacterRotation(FRotator(0.f, degrees, 0.f));

		OnInputPickup();
	}
}

void AKickboxersPlayerController::OnInputReady()
{
	if (AKBPlayerState * state = Cast<AKBPlayerState>(PlayerState))
	{
		state->SetPlayerReady_Server(!state->IsPlayerReady());
	}

	// Also using this for Restart right now.
	if (IsLocalController() && HasAuthority())
	{
		AKickboxersGameMode * gameMode = Cast<AKickboxersGameMode>(GetWorld()->GetAuthGameMode());
		if (gameMode && gameMode->HasMatchEnded())
		{
			gameMode->RestartGame();
		}
	}
}

void AKickboxersPlayerController::OnReadyStateChanged(AKBPlayerState * const readyPlayerState, bool isReady)
{
	//bPlayerIsWaiting = isReady;

	OnReadyStateChanged_Client(isReady);
}

void AKickboxersPlayerController::OnReadyStateChanged_Client_Implementation(bool isReady)
{
	//bPlayerIsWaiting = isReady;

	if (AKBHUD * hud = GetHUD<AKBHUD>())
	{
		hud->OnReadyStateChanged(isReady);
	}
}

void AKickboxersPlayerController::OnStartCountdown_Client_Implementation()
{
	if (AKBHUD * hud = GetHUD<AKBHUD>())
	{
		hud->OnStartCountdown();
	}
}

void AKickboxersPlayerController::OnCountdownAborted_Client_Implementation()
{
	if (AKBHUD * hud = GetHUD<AKBHUD>())
	{
		hud->OnCountdownAborted();
	}
}

void AKickboxersPlayerController::UpdateBoxesMatched(int32 matched)
{
	if (AKBHUD * hud = GetHUD<AKBHUD>())
	{
		hud->UpdateBoxesMatched(matched);
	}
}

void AKickboxersPlayerController::OnPlayerWon_Client_Implementation()
{
	if (AKBHUD * hud = GetHUD<AKBHUD>())
	{
		hud->OnPlayerWon();
	}
}

void AKickboxersPlayerController::OnPlayerLost_Client_Implementation()
{
	if (AKBHUD * hud = GetHUD<AKBHUD>())
	{
		hud->OnPlayerLost();
	}
}

void AKickboxersPlayerController::OnRoundComplete_Client_Implementation()
{
	if (AKBHUD * hud = GetHUD<AKBHUD>())
	{
		hud->OnRoundComplete();
	}
}

bool AKickboxersPlayerController::ForceGridMove(ECardinalDirection dir)
{
	if (AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>())
	{
		if (AKBGrid * grid = gameState->GetGrid())
		{
			if (AKickboxersCharacter * character = GetPawn<AKickboxersCharacter>())
			{
				// Should we grid move?
				if (grid->IsLocationWithinGrid(GetPawn()->GetActorLocation()))
				{
					const FIntPoint playerCell = grid->GetCellFromWorldLocation(GetPawn()->GetActorLocation());
					const FIntPoint targetCell = playerCell + UKBUtility::CardinalToPoint(dir);

					if (MovingOnGrid)
					{
						// Already in the process of moving, queue it
						QueuedGridMove = true;
						QueuedGridLoc = grid->GetCellLocation(targetCell);
					}
					else
					{
						// Move immediately
						/*MovingOnGrid = true;
						TargetGridLocation = grid->GetCellLocation(targetCell);
						TargetGridCell = targetCell;
						CurrentGridMoveDir = dir;*/
						StartGridMoveInDirection(dir);

						UpdateGridMovement(); // Don't wait for next tick
					}

					return true;
				}
			}
		}
	}

	return false;
}