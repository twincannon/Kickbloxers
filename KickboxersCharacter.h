// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KBDefinitions.h"
#include "KickboxersCharacter.generated.h"

class AKBBox;
class AKBBoxPreview;
class UKBGridCellComponent;

UCLASS(Blueprintable)
class AKickboxersCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AKickboxersCharacter(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, EditAnywhere)
	UKBGridCellComponent * GridComponent;

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* newController) override;
	virtual void Destroyed() override; // Faster than BeginDestroy
	virtual void FellOutOfWorld(const UDamageType& dmgType) override;

	UFUNCTION()
	void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

	UFUNCTION()
	void OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);


	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns CursorToWorld subobject **/
	FORCEINLINE class UDecalComponent* GetCursorToWorld() { return CursorToWorld; }

	AKBBox * DoKickBoxTrace() const;
	void UpdateBoxKickPreview();

	// If we're currently in a kicking state for purpose of animation
	UPROPERTY(Category = "Kickboxers", BlueprintReadWrite, ReplicatedUsing = "OnRep_AnimState")
	TEnumAsByte<EPlayerAnimState> AnimState = EPlayerAnimState::IDLE;

	void OnKick();

	UFUNCTION(Server, Unreliable)
	void OnPickup_Server();

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void ShowLiftedBoxPreview_Multicast(TSubclassOf<AKBBox> boxClass);

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void HideLiftedBoxPreview_Multicast();

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	TEnumAsByte<EPlayerAnimState> GetAnimState() const { return AnimState; }

	UFUNCTION(Category = "Kickboxers", BlueprintCallable)
	void KickAnimNotifyCallback();

	UFUNCTION(Category = "Kickboxers", BlueprintCallable)
	void KickEndNotifyCallback();

	UFUNCTION(Category = "Kickboxers", BlueprintCallable)
	void PickupNotifyCallback();

	//UFUNCTION()
	void OnBoxKicked(AKBBox * box);

	ECardinalDirection GetFacingDirection(bool relativeToCamera = false) const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_AnimState();

	void OnCharacterStuck();
	void OnCharacterUnstuck();
	bool IsCharacterStuck() const;

	void TryPlaceLiftedBox();

	void TryPickupBox();

	bool GetPushesEntireRows() const { return PushesEntireRows; }

	void PushCharacterAway(ECardinalDirection pushDir);

protected:
	bool PushesEntireRows = true;

	UPROPERTY()
	AKBBoxPreview * BoxPreview = nullptr;

	UPROPERTY(BlueprintReadOnly)
	AKBBoxPreview * LiftedBoxPreview = nullptr;

	FTimerHandle StuckTimer;
	FTimerHandle PreviewTimer;

	UFUNCTION(Client, Reliable)
	void SetPreviewTimer_Client();

	void KillStuckCharacter();
	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnStuckKilled();
	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnStuck();
	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnUnstuck();


	void SetAnimState(EPlayerAnimState newAnimState);
	UFUNCTION(Unreliable, Server)
	void SetAnimState_Server(EPlayerAnimState newAnimState);

	UFUNCTION(Unreliable, Server)
	void SetRotationEnabled_Server(bool enabled);

public:
	void SetCharacterRotation(FRotator rot);

	UFUNCTION(Unreliable, Server)
	void SetCharacterRotation_Server(FRotator rot);

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** A decal that projects to the cursor location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* CursorToWorld;
};

