// Fill out your copyright notice in the Description page of Project Settings.


#include "KBGameInstance.h"
#include "Kickboxers.h"
#include "Online.h"
#include "OnlineSubsystem.h"

void UKBGameInstance::Init()
{
	Super::Init();

	IOnlineSubsystem* SubSystem = IOnlineSubsystem::Get();
	IOnlineSessionPtr SessionInterface = SubSystem->GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UKBGameInstance::OnCreateSessionComplete);
		SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UKBGameInstance::OnDestroySessionComplete);
		SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UKBGameInstance::OnFindSessionsComplete);
		SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UKBGameInstance::OnJoinSessionsComplete);
		
	}
}

void UKBGameInstance::OnCreateSessionComplete(FName sessionName, bool successful)
{

}
void UKBGameInstance::OnDestroySessionComplete(FName sessionName, bool successful)
{

}
void UKBGameInstance::OnFindSessionsComplete(bool successful)
{

}
void UKBGameInstance::OnJoinSessionsComplete(FName sessionName, EOnJoinSessionCompleteResult::Type result)
{

}
