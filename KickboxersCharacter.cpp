// Copyright Epic Games, Inc. All Rights Reserved.

#include "KickboxersCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kickboxers.h"
#include "KBDefinitions.h"
#include "Actors/KBBox.h"
#include "Actors/KBBoxPreview.h"
#include "KickboxersPlayerController.h"
#include "Components/KBCharacterMovement.h"
#include "Components/KBGridCellComponent.h"
#include "Net/UnrealNetwork.h"
#include "KBGameState.h"
#include "KBPlayerState.h"
#include "Actors/KBGrid.h"
#include "Kismet/GameplayStatics.h"
#include "KickboxersGameMode.h"
#include "Actors/KBSprout.h"
#include "KBUserSettings.h"
#include "KBUtility.h"

AKickboxersCharacter::AKickboxersCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UKBCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(32.f, 96.0f);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AKickboxersCharacter::OnCapsuleBeginOverlap);
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AKickboxersCharacter::OnCapsuleHit);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->bCanWalkOffLedges = false;
	//GetCharacterMovement()->LedgeCheckThreshold = 500.f;// BOX_HEIGHT;

	//todo: figure out a way to get the character to fully rotate to the direction they last input
	//see UCharacterMovementComponent::PhysicsRotation
	
	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	if (GetMesh())
	{
		GetMesh()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}

	GridComponent = CreateDefaultSubobject<UKBGridCellComponent>(TEXT("GridComponent"));

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AKickboxersCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	if (HasAuthority() && LiftedBoxPreview == nullptr)
	{
		if (AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>())
		{
			if (gameState->GameSettings.AutoSpawnBoxAboveHead)
			{
				if (AKickboxersGameMode * gameMode = GetWorld()->GetAuthGameMode<AKickboxersGameMode>())
				{
					ShowLiftedBoxPreview_Multicast(gameMode->GetRandomBoxClass());
				}
			}
		}
	}
}

void AKickboxersCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AKickboxersCharacter::PossessedBy(AController* newController)
{
	Super::PossessedBy(newController);

	if (IsLocallyControlled())
	{
		if (PreviewTimer.IsValid() == false)
		{
			GetWorld()->GetTimerManager().SetTimer(PreviewTimer, this, &AKickboxersCharacter::UpdateBoxKickPreview, 0.2f, true);
		}
	}
	else
	{
		SetPreviewTimer_Client();
	}
}

void AKickboxersCharacter::SetPreviewTimer_Client_Implementation()
{
	if (IsLocallyControlled() && PreviewTimer.IsValid() == false)
	{
		GetWorld()->GetTimerManager().SetTimer(PreviewTimer, this, &AKickboxersCharacter::UpdateBoxKickPreview, 0.2f, true);
	}
}

void AKickboxersCharacter::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComaponent, class AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	
}

void AKickboxersCharacter::OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

}

AKBBox * AKickboxersCharacter::DoKickBoxTrace() const
{
	FCollisionQueryParams collisionQueryParams;
	collisionQueryParams.bIgnoreBlocks = false;
	collisionQueryParams.AddIgnoredActor(this);

	//FHitResult lineTraceHit;
	FVector traceStart = GetActorLocation();

	if (GetMesh())
	{
		FVector boneLoc = GetMesh()->GetBoneLocation("thigh_r"); // using thigh instead
		if (boneLoc != FVector::ZeroVector)
		{
			traceStart = boneLoc;// -GetActorForwardVector() * 90.f; // The foot is already extended at this point, so walk back a bit so we're tracing the entire leg
		}
	}

	traceStart.Z -= 20.f; // Boxes are only half our height so lower the trace a bit

	FVector traceEnd = traceStart + CardinalToVector(GetFacingDirection()) * 100.f; // Magic number, extent of trace (base box width for now)

	TArray<FHitResult> hits;
	FCollisionShape shape = FCollisionShape::MakeBox(FVector(20.f));
	
	//GetWorld()->bDebugDrawAllTraceTags = true;

	bool didHit = GetWorld()->SweepMultiByChannel(hits, traceStart, traceEnd, FQuat::Identity, ECollisionChannel::ECC_Pawn, shape, collisionQueryParams);

	//GetWorld()->bDebugDrawAllTraceTags = false;

	if (didHit)
	{
		// TODO: Make a Kickable interface and check if actor implements that etc.
		AKBBox * closestBox = nullptr;

		for (auto& hit : hits)
		{
			AKBBox * box = Cast<AKBBox>(hit.GetActor());
			if (IsValid(box) && box->IsValidForMatches() && !box->IsBoxMoving())
			{
				if (closestBox == nullptr || GetDistanceTo(box) < GetDistanceTo(closestBox))
				{
					closestBox = box;
				}
			}
		}

		return closestBox;
	}

	return nullptr;
}

void AKickboxersCharacter::UpdateBoxKickPreview()
{
	AKBGameState * state = GetWorld()->GetGameState<AKBGameState>();
	if (state && IsValid(state->GetGrid())) // Might be null if this is called too early as we rely on it being replicated down for clients
	{
		AKBBox * targetBox = DoKickBoxTrace();
		if (IsValid(targetBox))
		{
			EBoxMoveType moveType = EBoxMoveType::DEFAULT;
			FIntPoint movePoint = FIntPoint::NoneValue;
			bool canMove = targetBox->GetBoxMoveForDir(GetFacingDirection(), this, movePoint, moveType);
			if (canMove)
			{
				const FVector previewLoc = state->GetGrid()->GetCellLocation(movePoint) + FVector(0.f, 0.f, targetBox->GetBoxCollision()->GetScaledBoxExtent().Z);
				if (IsValid(BoxPreview))
				{
					BoxPreview->SetupPreview(targetBox, true);
					BoxPreview->SetActorLocation(previewLoc);
				}
				else
				{
					FTransform spawnTransform = FTransform(previewLoc);
					AKBBoxPreview * previewBox = GetWorld()->SpawnActorDeferred<AKBBoxPreview>(AKBBoxPreview::StaticClass(), spawnTransform, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					if (previewBox)
					{
						previewBox->SetupPreview(targetBox, true);
						UGameplayStatics::FinishSpawningActor(previewBox, spawnTransform);
						BoxPreview = previewBox;
					}
				}
			}
			else if (IsValid(BoxPreview))
			{
				BoxPreview->Destroy();
			}
		}
		else
		{
			if (IsValid(BoxPreview))
			{
				BoxPreview->Destroy();
			}
		}
	}
}

void AKickboxersCharacter::OnKick()
{
	// Do DegreesToCardinal here to effectively clamp our rotation to cardinal axis
	ECardinalDirection dir = DegreesToCardinalDir(GetActorRotation().Yaw);
	float yaw = CardinalToDegrees(dir);

	// We need to call this to get SetActorRotation to work 100% of the time, not sure why.
	GetCharacterMovement()->FlushServerMoves();

	// Set actor rotation to be cardinally aligned to the nearest direction
	FRotator newRot = FRotator(0.f, yaw, 0.f);
	SetCharacterRotation(newRot);
	
	SetAnimState(EPlayerAnimState::KICKING);

	// This function is called from our controller so we need to make sure to tell server about this
	if (!HasAuthority() && GetLocalRole() == ENetRole::ROLE_AutonomousProxy)
	{
		// Tell server about our animstate change
		SetAnimState_Server(EPlayerAnimState::KICKING);
	}
}

void AKickboxersCharacter::SetCharacterRotation(FRotator rot)
{
	SetActorRotation(rot);

	if (!HasAuthority() && GetLocalRole() == ENetRole::ROLE_AutonomousProxy)
	{
		SetCharacterRotation_Server(rot);
	}
}

void AKickboxersCharacter::SetCharacterRotation_Server_Implementation(FRotator rot)
{
	SetActorRotation(rot);
}

void AKickboxersCharacter::OnPickup_Server_Implementation()
{
	if (LiftedBoxPreview)
	{
		TryPlaceLiftedBox();
	}
	else
	{
		if (AKBGameState * gameState = GetWorld()->GetGameState<AKBGameState>())
		{
			// Check for alternative pickup logic
			if (gameState->GameSettings.PlayerPickupGrowsSprout)
			{
				if (AKBGrid * grid = gameState->GetGrid())
				{
					FIntPoint cell = grid->GetCellFromWorldLocation(GetActorLocation());
					FIntPoint frontCell = cell + UKBUtility::CardinalToPoint(GetFacingDirection());
					if (AKBSprout * sprout = grid->GetSproutInCell(frontCell))
					{
						if (grid->IsCellClear(frontCell, { this }) && grid->IsPointWithinGrid(frontCell) && grid->IsSameRegion(cell, frontCell))
						{
							grid->SpawnBoxInCell(frontCell, sprout->GetBoxToSpawnWhenGrown(), EBoxMoveType::PLACED, this);
							sprout->OnPicked_Multicast();
						}
					}
				}
			}
			else
			{
				TryPickupBox();
			}
		}

		//AKickboxersGameMode * gameMode = GetWorld()->GetAuthGameMode<AKickboxersGameMode>();
		//ShowLiftedBoxPreview_Multicast(gameMode->GetRandomBoxClass());
	}
}

void AKickboxersCharacter::TryPickupBox()
{
	if (AKBGameState * state = GetWorld()->GetGameState<AKBGameState>())
	{
		if (AKBGrid * grid = state->GetGrid())
		{
			FIntPoint cell = grid->GetCellFromWorldLocation(GetActorLocation());
			ECardinalDirection facing = GetFacingDirection();
			FIntPoint frontCell = cell + UKBUtility::CardinalToPoint(facing);

			// Make sure it's the same region as us so we can't just steal neighbors stuff
			bool sameRegion = grid->IsSameRegion(frontCell, cell);

			// Try picking up a box in front
			if (AKBBox * box = grid->GetBoxInCell(frontCell))
			{
				if (sameRegion)
				{
					ShowLiftedBoxPreview_Multicast(box->GetClass());
					box->RemoveBox();
				}
			}
			else // Look for a sprout in our current cell
			{
				if (AKBSprout * sprout = grid->GetSproutInCell(cell))
				{
					ShowLiftedBoxPreview_Multicast(sprout->GetBoxToSpawnWhenGrown());
					sprout->OnPicked_Multicast();
				}
				else if (sameRegion) // Look for a sprout in front
				{
					sprout = grid->GetSproutInCell(frontCell);
					if (IsValid(sprout))
					{
						ShowLiftedBoxPreview_Multicast(sprout->GetBoxToSpawnWhenGrown());
						sprout->OnPicked_Multicast();
					}
				}
			}
		}
	}
}

void AKickboxersCharacter::ShowLiftedBoxPreview_Multicast_Implementation(TSubclassOf<AKBBox> boxClass)
{
	if (*boxClass && !LiftedBoxPreview)
	{
		FTransform spawnTransform = FTransform(FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
		AKBBoxPreview * previewBox = GetWorld()->SpawnActorDeferred<AKBBoxPreview>(AKBBoxPreview::StaticClass(), spawnTransform, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (previewBox)
		{
			AKBBox * boxCDO = Cast<AKBBox>(boxClass->GetDefaultObject());
			previewBox->SetupPreview(boxCDO, false);
			previewBox->SetBoxClass(boxClass);
			UGameplayStatics::FinishSpawningActor(previewBox, spawnTransform);
			LiftedBoxPreview = previewBox;

			LiftedBoxPreview->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
}

void AKickboxersCharacter::HideLiftedBoxPreview_Multicast_Implementation()
{
	if (LiftedBoxPreview)
	{
		LiftedBoxPreview->Destroy();
		LiftedBoxPreview = nullptr;
	}
}

void AKickboxersCharacter::TryPlaceLiftedBox()
{
	UKBUserSettings * settings = Cast<UKBUserSettings>(GEngine->GameUserSettings);

	if (LiftedBoxPreview && *LiftedBoxPreview->GetBoxClass())
	{
		if (AKBGameState * state = GetWorld()->GetGameState<AKBGameState>())
		{
			if (AKBGrid * grid = state->GetGrid()/*; IsValid(grid)*/)
			{
				FIntPoint cell = grid->GetCellFromWorldLocation(GetActorLocation());
				const ECardinalDirection facingDir = GetFacingDirection();
				FIntPoint targetCell = cell + UKBUtility::CardinalToPoint(facingDir);
				if (grid->IsCellClear(targetCell, { this }) && grid->IsPointWithinGrid(targetCell) && grid->IsSameRegion(cell, targetCell))
				{
					AKBBox * box = grid->SpawnBoxInCell(targetCell, LiftedBoxPreview->GetBoxClass(), EBoxMoveType::PLACED, this);
					HideLiftedBoxPreview_Multicast();

					// Check if player should auto kick boxes after placing them (i.e. "throwing" placed boxes)
					if (state->GameSettings.PlayerAutoKicksBoxAfterPlacing)
					{
						// Check if the box can actually be kicked without immediately being blocked
						// to prevent placing a block in front of a wall and immediately having it be crushed etc.
						if (grid->IsCellClear(targetCell + UKBUtility::CardinalToPoint(facingDir)))
						{
							OnBoxKicked(box);
						}
					}
				}
			}
		}
	}
}

void AKickboxersCharacter::SetRotationEnabled_Server_Implementation(bool enabled)
{
	GetCharacterMovement()->bOrientRotationToMovement = enabled;
}

void AKickboxersCharacter::SetAnimState(EPlayerAnimState newAnimState)
{
	AnimState = newAnimState;

	// Lock rotation during kicking so you can't fling your leg around
	GetCharacterMovement()->bOrientRotationToMovement = (AnimState != EPlayerAnimState::KICKING);
}

void AKickboxersCharacter::SetAnimState_Server_Implementation(EPlayerAnimState newAnimState)
{
	// The important thing is this function is Server RPC - we can just use the same function now that we're here
	SetAnimState(newAnimState);
}

void AKickboxersCharacter::KickAnimNotifyCallback()
{
	AKBBox * closestBox = DoKickBoxTrace();

	if (IsValid(closestBox))
	{
		OnBoxKicked(closestBox);
	}
}

void AKickboxersCharacter::KickEndNotifyCallback()
{
	SetAnimState(EPlayerAnimState::IDLE);
	
	if (!HasAuthority() && GetLocalRole() == ENetRole::ROLE_AutonomousProxy)
	{
		SetAnimState_Server(EPlayerAnimState::IDLE);
	}
}

void AKickboxersCharacter::PickupNotifyCallback()
{
	// TODO: use DoKickBoxTrace, make it ambiguous and make it so i can change the params of the trace size (and stop using thigh_r)
	// this is already mostly a copy of it anyway it seems.

	FCollisionQueryParams collisionQueryParams;
	collisionQueryParams.bIgnoreBlocks = false;
	collisionQueryParams.AddIgnoredActor(this);

	FVector traceStart = GetActorLocation();
	traceStart.Z -= 20.f; // Boxes are only half our height so lower the trace a bit

	FVector traceEnd = traceStart + GetActorForwardVector() * 80.f; // Add a bit extra for lenience

	TArray<FHitResult> hits;
	FCollisionShape shape = FCollisionShape::MakeBox(FVector(20.f));

	//GetWorld()->bDebugDrawAllTraceTags = true;

	bool didHit = GetWorld()->SweepMultiByChannel(hits, traceStart, traceEnd, FQuat::Identity, ECollisionChannel::ECC_Pawn, shape, collisionQueryParams);

	if (didHit)
	{
		AKBBox * closestBox = nullptr;
		for (auto& hit : hits)
		{
			AKBBox * box = Cast<AKBBox>(hit.GetActor());
			if (IsValid(box))
			{
				if (closestBox == nullptr || GetDistanceTo(box) < GetDistanceTo(closestBox))
				{
					closestBox = box;
				}
			}
		}

		if (IsValid(closestBox))
		{
			//OnLifted();
			GridComponent->ResetGridCell();
			closestBox->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			//SetLiftedBox(closestBox);
			// do an interface here
		}
	}
}

void AKickboxersCharacter::OnBoxKicked(AKBBox * box)
{
	box->KickBox(GetFacingDirection(), this);
}

ECardinalDirection AKickboxersCharacter::GetFacingDirection(bool relativeToCamera /* = false */) const
{
	if (!relativeToCamera)
	{
		// Add 90 degrees here to translate from character alignment (+X forward) to grid alignment (-Y north)
		return DegreesToCardinalDir(GetActorRotation().Yaw + 90.f);
	}
	else
	{
		if (Controller && Controller->GetViewTarget())
		{
			// Get local facing by subtracting out the viewtarget rotation
			return DegreesToCardinalDir(ClampDegrees(GetActorRotation().Yaw - Controller->GetViewTarget()->GetActorRotation().Yaw));
		}
		else
		{
			ensureMsgf(false, TEXT("AKickboxersCharacter::GetFacingDirectionLocal - shouldn't be here!"));
			return ECardinalDirection::NORTH;
		}
	}
}

void AKickboxersCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// "It's important that you understand that the whole replication process only works from Server to Client and NOT the other way round."

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(AKickboxersCharacter, AnimState, COND_None, REPNOTIFY_Always);
}

void AKickboxersCharacter::OnRep_AnimState()
{
	// This is only ever repped from server to client, so we never HasAuthority() (originally wanted to handle SetAnimState stuff here)
}

void AKickboxersCharacter::Destroyed()
{
	PreviewTimer.Invalidate();

	if (IsValid(BoxPreview))
	{
		BoxPreview->Destroy();
	}

	// Call super here to set the actor as uninitialized, which our playerstate checks
	Super::Destroyed();

	if (!IsTemplate() && !HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		if (HasAuthority())
		{
			if (AKBGameState * state = GetWorld()->GetGameState<AKBGameState>())
			{
				state->OnPlayerDied();
			}
		}
	}
}

void AKickboxersCharacter::FellOutOfWorld(const UDamageType& dmgType)
{
	Super::FellOutOfWorld(dmgType);
}

void AKickboxersCharacter::OnCharacterStuck()
{
	if (IsCharacterStuck() == false)
	{
		GetWorld()->GetTimerManager().SetTimer(StuckTimer, this, &AKickboxersCharacter::KillStuckCharacter, 5.f, false);
		OnStuck();
	}
}

void AKickboxersCharacter::OnCharacterUnstuck()
{
	if (IsCharacterStuck())
	{
		GetWorld()->GetTimerManager().ClearTimer(StuckTimer);
		OnUnstuck();
	}
}

void AKickboxersCharacter::KillStuckCharacter()
{
	OnStuckKilled();
	Destroy();
}

bool AKickboxersCharacter::IsCharacterStuck() const
{
	return GetWorld()->GetTimerManager().IsTimerActive(StuckTimer);
}

void AKickboxersCharacter::PushCharacterAway(ECardinalDirection pushDir)
{
	AKickboxersPlayerController * controller = Cast<AKickboxersPlayerController>(Controller);
	if (IsValid(controller))
	{
		// Try using grid movement
		if (!controller->ForceGridMove(pushDir))
		{
			// If we got here, just launch the character like we used to
			const FVector vecDir = CardinalToVector(pushDir) * 1000.f + FVector(0.f, 0.f, 10.f);
			LaunchCharacter(vecDir, true, true);
		}
	}

	//GetCharacterMovement()->AddImpulse(pushDir, 300.f, true);
}