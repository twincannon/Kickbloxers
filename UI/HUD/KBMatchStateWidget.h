// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KBMatchStateWidget.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API UKBMatchStateWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnPlayerWon();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnPlayerLost();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void OnRoundComplete();
};
