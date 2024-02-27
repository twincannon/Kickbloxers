// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "KBDefinitions.h"
#include "KickboxersGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMatchSetReady);

class AKBGrid;
class AKBBox;
class AKBSprout;

UCLASS(minimalapi)
class AKickboxersGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AKickboxersGameMode();

	virtual void BeginPlay() override;
	// whats the difference between this and StartPlay?

	virtual void StartMatch() override;

	virtual void EndMatch() override;

	virtual void PostSeamlessTravel() override;

	virtual void StartBoxTimer();

	virtual void PostLogin(APlayerController * newPlayer) override;

	virtual bool ReadyToStartMatch_Implementation() override;

	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnMatchSetReady OnMatchSetReady;

	UFUNCTION(Category = "Kickboxers", BlueprintCallable)
	void SetPlayerWon(APlayerController * winningPlayer);

	UFUNCTION(exec)
	void KB_SetBoxSpawningEnabled(bool enabled);

	// Spawns box in first region of grid
	UFUNCTION(exec)
	void KB_SpawnBox(int32 x, int32 y);

	UFUNCTION(exec)
	void KB_SetBoxSpawnRate(float rate);

	UFUNCTION(exec)
	void KB_SetBoxWallPushRadius(float radius);
	UFUNCTION(exec)
	void KB_SetSproutGrowTime(float time);
	UFUNCTION(exec)
	void KB_SetPlayerPickupEnabled(bool enabled);
	UFUNCTION(exec)
	void KB_SetPlayerKickEnabled(bool enabled);
	UFUNCTION(exec)
	void KB_SetPlayerPickupGrowsSprout(bool enabled);

	inline virtual int32 GetMinimumEmptyCells() const { return 3; }

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	TSubclassOf<AKBBox> GetRandomBoxClass() const;

	TSubclassOf<AKBSprout> GetRandomSproutClass() const;

protected:
	// Box classes - to be filled out in blueprint child class
	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly)
	TArray<TSubclassOf<AKBBox>> BoxClasses;

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly)
	TArray<TSubclassOf<AKBSprout>> SproutClasses;

	// Default settings for this game mode
	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	FGameSettings GameSettings;

	UFUNCTION()
	void SpawnBox();

	UFUNCTION()
	void SetMatchReady();

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly)
	float CountDownLength = 3.f;

	// Timer used when number of required players are ready, but there are still optional slots that can be filled
	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly)
	float CountDownLengthShortLong = 10.f;

	UFUNCTION()
	void DoLongMatchCountdown();
	UFUNCTION()
	void DoStartMatchCountdown();
	UFUNCTION()
	void CheckMatchCountdown(int32 playerNum, int32 readyNum, int32 required, int32 optional);
	UFUNCTION()
	void NoPlayersAlive();

	FTimerHandle BoxSpawnTimer;

protected:
	bool MatchReady = false;

private:
	bool CountingDown = false;

	FTimerHandle MatchStartCountdown;

};



