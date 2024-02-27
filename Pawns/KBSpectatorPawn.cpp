// Copyright Puny Human LLC 2020


#include "KBSpectatorPawn.h"
#include "KickboxersPlayerController.h"

void AKBSpectatorPawn::BeginPlay()
{
	Super::BeginPlay();
}

void AKBSpectatorPawn::PossessedBy(AController* newController)
{
	// Sole reason we need this spectator pawn because clients have no function called
	// on them post-seamless travel, thanks ue4

	Super::PossessedBy(newController);

	if (AKickboxersPlayerController * kbc = Cast<AKickboxersPlayerController>(newController))
	{
		kbc->HideLoadingScreen();
	}
}