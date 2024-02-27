// Copyright Puny Human LLC 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "KBDefinitions.h"
#include "KBUserSettings.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API UKBUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
public:
	// If we should override gamemode settings with these custom game settings
	UPROPERTY(BlueprintReadWrite, Category = "Kickboxers")
	bool OverrideGamemodeSettings = false;


	// Server settings (gamemode applies the hosts settings and are applied to all players)
	UPROPERTY(config, BlueprintReadWrite, Category = "Kickboxers")
	FGameSettings GameSettings;

	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void ResetGameSettings();

	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void ResetClientSettings();


	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void SetBoxPushWallRadius(float value) { GameSettings.BoxPushWallRadius = value; }

	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void SetSpawnSproutsOnMatches(bool enabled) { GameSettings.SpawnSproutsOnMatches = enabled; }

	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void SetSproutSpawnRate(float value) { GameSettings.SproutSpawnRate = value; }

	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void SetSproutGrowTime(float value) { GameSettings.SproutGrowTime = value; }

	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void SetPlayerPickupEnabled(bool enabled) { GameSettings.PlayerPickupEnabled = enabled; }

	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void SetPlayerKickEnabled(bool enabled) { GameSettings.PlayerKickEnabled = enabled; }

	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void SetPlayerPickupGrowsSprout(bool enabled) { GameSettings.PlayerPickupGrowsSprout = enabled; }

	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void SetAutoSpawnBoxAboveHead(bool enabled) { GameSettings.AutoSpawnBoxAboveHead = enabled; }

	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void SetPlayerAutoKicksBoxAfterPlacing(bool enabled) { GameSettings.PlayerAutoKicksBoxAfterPlacing = enabled; }


	// Client settings (applied only to the client)
	UPROPERTY(config, BlueprintReadWrite, Category = "Kickboxers")
	FClientSettings ClientSettings;

	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void SetTapToLook(bool enabled) { ClientSettings.TapToLook = enabled; }

	UFUNCTION(BlueprintCallable, Category = "Kickboxers")
	void SetTapToLookTimer(float value) { ClientSettings.TapToLookTimer = value; }	
};
