// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KBDefinitions.h"
#include "KBBox.generated.h"

class USceneComponent;
class UBoxComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class AKickboxersCharacter;
class AKBGrid;
class UKBGridCellComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBoxFinishedMovingDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBoxMovedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBoxPickedUpDelegate);

UCLASS()
class KICKBOXERS_API AKBBox : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AKBBox();

	// Multicast as the spawning comes from Server
	UFUNCTION(Unreliable, NetMulticast)
	void SetupBox(AKBGrid * grid, FIntPoint startPoint, EBoxMoveType initialMoveType);

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	USceneComponent * BoxRoot;

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	UBoxComponent * BoxCollision;

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	UBoxComponent * PlayerCheck;

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	UStaticMeshComponent * BoxMesh;

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	UTextRenderComponent * DebugText;

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	UKBGridCellComponent * GridComponent;

	UBoxComponent * GetBoxCollision() const { return BoxCollision; }

	UPROPERTY(Category = "Kickboxers", BlueprintCallable)
	FOnBoxFinishedMovingDelegate OnBoxFinishedMoving;

	FOnBoxMovedDelegate OnBoxMoved;
	FOnBoxPickedUpDelegate OnBoxPickedUp;

	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

	UFUNCTION()
	void OnBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnPlayerCheckOverlap(UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

	bool CanBeKicked() const;

	// Pushes this box as far as it can go
	void KickBox(ECardinalDirection pushDir, AKickboxersCharacter * const pusher);

	bool GetBoxMoveForDir(ECardinalDirection pushDir, AKickboxersCharacter * const pusher, FIntPoint &outMoveToPoint, EBoxMoveType &outMoveType) const;

	void KickBoxRow(ECardinalDirection pushDir, AKickboxersCharacter * const pusher);
	// Pushes this box and all boxes behind it a single unit in target direction
	bool PushBoxRow(ECardinalDirection pushDir, AKickboxersCharacter * const pusher);

	// Forcibly and recursively moves all boxes along a given direction (for walls moving boxes)
	void RecursiveBoxMove(ECardinalDirection moveDir, TArray<AKBBox*>& checkedBoxes, AKickboxersCharacter * const pusher = nullptr);

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	FIntPoint GetBoxCell() const;

	bool MatchesType(AKBBox * otherBox) const;

	// If this box should be considered in searching for matches
	bool IsValidForMatches() const;

	bool CanBeMoved() const { return IsValidForMatches() && NumTilesPerMove > 0; }

	bool IsBoxOnFloor() const { return (BoxZ == 0); }

	void OnBoxMatched_Native();

	UFUNCTION(Unreliable, NetMulticast)
	void DoBoxMatchStartEffects_Multicast();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnBoxMatchStart();

	UFUNCTION(Reliable, NetMulticast)
	void OnMatchTimerFinish_Multicast();

	UFUNCTION(Unreliable, NetMulticast)
	void OnBoxDestroyed_Multicast();

	// Destroy box and let blueprints play effects or perform logic as needed
	UFUNCTION(Category = "Kickboxers", BlueprintCallable)
	void DestroyBox();

	void RemoveBox();

	void SetBoxCell(FIntPoint newCell);

	bool IsFinishedSpawning() const;

	bool IsBoxMoving() const { return CurrentMoveType != EBoxMoveType::NONE; }

	AKickboxersCharacter * GetKicker() const { return Kicker.IsValid() ? Kicker.Get() : nullptr; }
	void SetKicker(AKickboxersCharacter * const newKicker);

	void SetBoxTargetLocation(FVector loc) { TargetLocation = loc; }
	FVector GetBoxTargetLocation() const { return TargetLocation; }

	FVector FinalTargetLocation;
	
	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void MoveBoxTo(FVector targetLoc, FIntPoint cellPoint, EBoxMoveType moveType = EBoxMoveType::DEFAULT);
	UFUNCTION(Category = "Kickboxers", BlueprintCallable)
	void MoveBoxTo_Internal(FIntPoint cellPoint, EBoxMoveType moveType = EBoxMoveType::DEFAULT);

	void SetBoxEnabled(bool enabled);

	// How many tiles this box moves when kicked
	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadWrite)
	int32 NumTilesPerMove = 100;

	// If this box can ever be matched with similar boxes (set false to make garbage box)
	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadWrite)
	bool CanBeMatched = true;

	// If, when kicked while stuck, this box will flip behind the player. Also determines if box is flipped when blocked while pushing (takes priority over crushing)
	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly)
	bool CanBeFlipped = false;

	// If this box crushes stuck blocks when kicked into them (relies on CanBeFlipped setting false)
	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, meta = (EditCondition = "!CanBeFlipped"))
	bool CrushesStuckBlocks = true;

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly)
	bool CanBeCrushed = true;

	int32 GetComboNum() const { return ComboNum; }
	void SetComboNum(int32 val) { ComboNum = val; }

protected:

	// TODO: on box matched nearby delegate - to clear garbage, ignite nearby tnt boxes, etc

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void ShuffleHeadBounceDirs();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 ComboNum = 0;

	// TODO: I want this to be FVector_NetQuantize100 but it breaks FMath::Lerp and other things?
	UPROPERTY(Replicated)
	FVector TargetLocation = FVector::ZeroVector;

	// Should this replicate...?
	UPROPERTY(Replicated)
	TEnumAsByte<EBoxMoveType> CurrentMoveType = EBoxMoveType::NONE;

	TArray<FIntPoint> HeadBounceDirs;

	//UFUNCTION()
	//void OnRep_BoxCell(const FIntPoint& oldCell);

	// Last character to kick this box (cleared when finished moving)
	TWeakObjectPtr<AKickboxersCharacter> Kicker;

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void PlaySpawnAnimation(FVector targetLoc);

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnBoxCantMove();

	// Called when the box is part of a successful match, returns how long the timer for clearing be (to play effects etc.)
	UFUNCTION(Category = "Kickboxers", BlueprintNativeEvent)
	float OnBoxMatched();

	// Called when box is cleared (i.e. it was matched and now the match timer is finished and the box will be destroyed). Returns how long to set the box lifespan (for playing final effects)
	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	float OnBoxCleared();

	// Called when this box is destroyed, returns how long the box should live for afterwards
	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	float OnBoxDestroyed();

	UFUNCTION()
	void OnBoxFinishMove();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void StopMoving();

private:
	int32 BoxZ = 0; // Higher Z means the box is on top of other boxes

	void VerifyBoxCell();


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
