// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "KBDefinitions.h"
#include "KBGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class KICKBOXERS_API UKBGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	
	void OnCreateSessionComplete(FName sessionName, bool successful);
	void OnDestroySessionComplete(FName sessionName, bool successful);
	void OnFindSessionsComplete(bool successful);
	void OnJoinSessionsComplete(FName sessionName, EOnJoinSessionCompleteResult::Type result);

};
