// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "KBHUD.generated.h"

class UKBBaseHudWidget;

/**
 * 
 */
UCLASS()
class KICKBOXERS_API AKBHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	void OnReadyStateChanged(bool isReady);
	void OnStartCountdown();
	void OnCountdownAborted();
	void UpdateBoxesMatched(int32 matched);
	void OnPlayerWon();
	void OnPlayerLost();
	void OnRoundComplete();

protected:
	// The class used for the fullscreen HUD during gameplay
	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly)
	TSubclassOf<UKBBaseHudWidget> BaseHudWidgetClass;

	UPROPERTY()
	UKBBaseHudWidget * HudRef;
};
