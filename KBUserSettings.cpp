// Copyright Puny Human LLC 2020


#include "KBUserSettings.h"

void UKBUserSettings::ResetGameSettings()
{
	GameSettings = FGameSettings();
}

void UKBUserSettings::ResetClientSettings()
{
	ClientSettings = FClientSettings();
}
