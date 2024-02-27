// Copyright Puny Human LLC 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KBSprout.generated.h"

class AKBBox;
class UKBGridCellComponent;

UCLASS()
class KICKBOXERS_API AKBSprout : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AKBSprout();

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	UKBGridCellComponent * GridComponent;

	void OnWatered_Internal();

	UFUNCTION(NetMulticast, Unreliable)
	void OnPicked_Multicast();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnWatered();

	UFUNCTION()
	void TryToGrow();

	UFUNCTION()
	void ManualGrowSprout();

	UFUNCTION(NetMulticast, Unreliable)
	void OnSprouted_Multicast();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnSprouted();

	UFUNCTION(NetMulticast, Unreliable)
	void OnGrowWarning_Multicast();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnGrowWarning();

	UFUNCTION(NetMulticast, Unreliable)
	void OnGrown_Multicast();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnGrown();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnPicked();

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	UStaticMesh * GetBoxMesh() const;

	FTimerHandle GrowHandle;
	FTimerHandle SproutHandle;
	FTimerHandle WarningHandle;

	// How long it takes this plant to "grow", automatically spawning its box (can be set to -1 to never spawn automatically)
//	UPROPERTY(Category = "Kickboxers", EditAnywhere)
//	float GrowTime = 30.f;

	// How long it takes before the plant sprouts showing what kind of box it will spawn
	UPROPERTY(Category = "Kickboxers", EditAnywhere)
	float SproutTime = 8.f;

	// If the plant regrows then the sprout doesn't get destroyed after blooming
	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly)
	bool Regrows = false;

	UPROPERTY(Category = "Kickboxers", EditAnywhere, BlueprintReadOnly, Replicated)
	TSubclassOf<AKBBox> BoxToSpawnWhenGrown;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	TSubclassOf<AKBBox> GetBoxToSpawnWhenGrown() const { return BoxToSpawnWhenGrown; }
};
