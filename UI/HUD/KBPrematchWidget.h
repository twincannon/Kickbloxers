// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KBPrematchWidget.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API UKBPrematchWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void DoCountdown();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void AbortCountdown();

	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void UpdateReadyState(bool isReady);
	
};
