// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Kickboxers : ModuleRules
{
	public Kickboxers(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.AddRange(new string[]
    {
            "Kickboxers/Actors",
            "Kickboxers/Components",
            "Kickboxers"
    });

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "NavigationSystem", "AIModule", "OnlineSubsystem", "﻿OnlineSubsystemSteam﻿" });
    }
}
