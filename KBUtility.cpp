// Fill out your copyright notice in the Description page of Project Settings.


#include "KBUtility.h"
#include "KBUserSettings.h"
#include "Actors/KBBox.h"
#include "KBGameState.h"

const UEnum* UKBUtility::FindGlobalEnum(const FString& enumName)
{
	// Using ANY_PACKAGE is very slow and once we've found an enum it's not going to change as UEnums are compiled into the executable and never destroyed
	static TMap<FString, UEnum*> cachedNames;

	UEnum** foundEnum = cachedNames.Find(enumName);

	if (foundEnum)
	{
		return *foundEnum;
	}

	// Failed to find in cache, look up manually
	UEnum* enumPtr = FindObjectSafe<UEnum>(ANY_PACKAGE, *enumName, true);
	if (enumPtr)
	{
		cachedNames.Add(enumName, enumPtr);
	}
	return enumPtr;
}

UKBUserSettings * UKBUtility::GetKBUserSettings()
{
	return Cast<UKBUserSettings>(GEngine->GetGameUserSettings());
}

UStaticMesh * UKBUtility::GetBoxMeshFromClass(TSubclassOf<AKBBox> boxClass)
{
	if (boxClass)
	{
		if (AKBBox * box = Cast<AKBBox>(boxClass->GetDefaultObject()))
		{
			return box->BoxMesh->GetStaticMesh();
		}
	}

	return nullptr;
}

AKBGameState * UKBUtility::GetKBGameState(const UObject* worldContextObject)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? Cast<AKBGameState>(World->GetGameState()) : nullptr;
}

bool UKBUtility::HasGameStarted(const UObject* worldContextObject)
{
	if (AKBGameState * gamestate = UKBUtility::GetKBGameState(worldContextObject))
	{
		return gamestate->HasGameStarted();
	}
	else
	{
		return false;
	}
}

bool UKBUtility::DoServerTravel(const UObject* worldContextObject, const FString& levelName)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		return World->ServerTravel(levelName);
	}

	return false;
}
