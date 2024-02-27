// Fill out your copyright notice in the Description page of Project Settings.

#include "KBHUD.h"
#include "UI/KBBaseHudWidget.h"
#include "UI/HUD/KBMatchStateWidget.h"
#include "UI/HUD/KBPrematchWidget.h"

void AKBHUD::BeginPlay()
{
	Super::BeginPlay();

	if (BaseHudWidgetClass)
	{
		HudRef = CreateWidget<UKBBaseHudWidget>(GetOwningPlayerController(), BaseHudWidgetClass);
		if (HudRef)
		{
			HudRef->AddToViewport();
		}
	}
}

void AKBHUD::Destroyed()
{
	if (HudRef)
	{
		HudRef->RemoveFromViewport();
	}

	Super::Destroyed();
}

void AKBHUD::OnReadyStateChanged(bool isReady)
{
	if (HudRef && HudRef->PreMatchWidget)
	{
		HudRef->PreMatchWidget->UpdateReadyState(isReady);
	}
}

void AKBHUD::OnStartCountdown()
{
	if (HudRef && HudRef->PreMatchWidget)
	{
		HudRef->PreMatchWidget->DoCountdown();
	}
}

void AKBHUD::OnCountdownAborted()
{
	if (HudRef && HudRef->PreMatchWidget)
	{
		HudRef->PreMatchWidget->AbortCountdown();
	}
}

void AKBHUD::UpdateBoxesMatched(int32 matched)
{
	if (HudRef)
	{
		HudRef->SetBoxesMatched(matched);
	}
}

void AKBHUD::OnPlayerWon()
{
	if (HudRef && HudRef->MatchStateWidget)
	{
		HudRef->MatchStateWidget->OnPlayerWon();
	}
}

void AKBHUD::OnPlayerLost()
{
	if (HudRef && HudRef->MatchStateWidget)
	{
		HudRef->MatchStateWidget->OnPlayerLost();
	}
}

void AKBHUD::OnRoundComplete()
{
	if (HudRef && HudRef->MatchStateWidget)
	{
		HudRef->MatchStateWidget->OnRoundComplete();
	}
}