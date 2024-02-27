// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KBBaseHudWidget.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API UKBBaseHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "Kickboxers", BlueprintImplementableEvent)
	void SetBoxesMatched(int32 matched);

	UPROPERTY(Category = "Kickboxers", BlueprintReadOnly, Meta = (BindWidget))
	class UKBMatchStateWidget * MatchStateWidget;

	UPROPERTY(Category = "Kickboxers", BlueprintReadOnly, Meta = (BindWidget))
	class UKBPrematchWidget * PreMatchWidget;
};
