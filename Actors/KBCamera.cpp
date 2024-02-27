// Fill out your copyright notice in the Description page of Project Settings.


#include "KBCamera.h"

void AKBCamera::BeginPlay()
{
	Super::BeginPlay();

	if (ForceUseCamera)
	{
		APlayerController * controller = GetWorld()->GetFirstPlayerController();
		if (controller)
		{
			controller->SetViewTarget(this);
		}
	}
}