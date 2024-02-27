// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KBDefinitions.h"
#include "KickboxersPlayerController.generated.h"

class UKBBaseHudWidget;
class AKBPlayerState;

UCLASS()
class AKickboxersPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AKickboxersPlayerController();

	void OnLoginToLobby();

	virtual void AcknowledgePossession(class APawn* p) override;

	virtual void ReceivedPlayer() override;

	bool IsReady = false;

	UFUNCTION(Client, Unreliable)
	void OnStartCountdown_Client();

	UFUNCTION(Client, Unreliable)
	void OnCountdownAborted_Client();

	void UpdateBoxesMatched(int32 matched);

	UFUNCTION(Client, Reliable)
	void OnPlayerWon_Client();
	UFUNCTION(Client, Reliable)
	void OnPlayerLost_Client();
	UFUNCTION(Client, Reliable)
	void OnRoundComplete_Client();

	bool IsMovingOnGrid() const { return MovingOnGrid; }

	// Forces a move in direction on grid, returns if successful
	bool ForceGridMove(ECardinalDirection dir);

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void ShowLoadingScreen();
	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void HideLoadingScreen();

protected:
	float MoveAngleOffset = 0.f;

	bool MovingOnGrid = false;
	bool PrevMovingOnGrid = false;
	ECardinalDirection CurrentGridMoveDir = ECardinalDirection::EAST;
	FVector TargetGridLocation = FVector::ZeroVector;
	FVector PreviousGridLoc = FVector::ZeroVector;

	bool QueuedGridMove = false;
	FVector QueuedGridLoc = FVector::ZeroVector;

	void UpdateGridMovement();

	bool PreviousMoveKeyHeld = false;

	FTimerHandle GridTurnHandle;
	FTimerHandle GridMoveBufferHandle;

	FTimerHandle PlayerFailedMoveTimer;

	// @CLEANUP: These functions (and this grid logic in general) should probably be in pawn / gridcomponent
	FIntPoint GetCharacterTargetGridCell() const;
	void SetCharacterTargetGridCell(FIntPoint newTargetCell);

	virtual void PostInitializeComponents() override;
	virtual void InitPlayerState() override;
	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;
	virtual void PostSeamlessTravel() override;

	void FindViewTarget();

	UFUNCTION()
	void OnReadyStateChanged(AKBPlayerState * const readyPlayerState, bool isReady);
	UFUNCTION(Client, Reliable)
	void OnReadyStateChanged_Client(bool isReady);

	// Begin PlayerController interface
	virtual void PlayerTick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	// End PlayerController interface

	FVector MovePawnBasedOnViewTarget(float axisAmount, float additionalAngle = 0.f);

	void OnInputNorth(float axisAmount);
	void OnInputEast(float axisAmount);

	template<ECardinalDirection pickupDir>
	void PickupInDirection();

	UFUNCTION(Server, Unreliable)
	void OnPlayerTriedMovingOutsideGrid_Server(AKBGrid * grid, FIntPoint targetCell);

	UFUNCTION(Server, Unreliable)
	void OnPlayerGridMoved_Server(ECardinalDirection dir, bool allowOutOfBounds);
	UFUNCTION(Client, Reliable)
	void CorrectGridMove_Client(FIntPoint playerCell, FVector targetLoc);

	void OnInputKick();
	void OnInputPickup();
	void OnInputReady();
	void OnInputAction();

	UFUNCTION(Server, Unreliable)
	void DoActionTrace_Server();
	void DoActionTrace();

	UFUNCTION(Server, Unreliable)
	void OnPlayerBumpedIntoObject_Server(AActor * blockingActor);
	void OnPlayerBumpedIntoObject(AActor * blockingActor);

	bool StartGridMoveInDirection(ECardinalDirection dir, bool allowOutOfBounds = false);
};


