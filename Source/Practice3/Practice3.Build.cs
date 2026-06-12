// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Practice3 : ModuleRules
{
	public Practice3(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Practice3",
			"Practice3/Variant_Platforming",
			"Practice3/Variant_Platforming/Animation",
			"Practice3/Variant_Combat",
			"Practice3/Variant_Combat/AI",
			"Practice3/Variant_Combat/Animation",
			"Practice3/Variant_Combat/Gameplay",
			"Practice3/Variant_Combat/Interfaces",
			"Practice3/Variant_Combat/UI",
			"Practice3/Variant_SideScrolling",
			"Practice3/Variant_SideScrolling/AI",
			"Practice3/Variant_SideScrolling/Gameplay",
			"Practice3/Variant_SideScrolling/Interfaces",
			"Practice3/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
