// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Practice : ModuleRules
{
    public Practice(ReadOnlyTargetRules Target) : base(Target)
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
            "Slate",
            "SlateCore",
            "OnlineSubsystem",
            "OnlineSubsystemSteam",
            "OnlineSubsystemUtils"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        PublicIncludePaths.AddRange(new string[] {
            "Practice",
            "Practice/Variant_Platforming",
            "Practice/Variant_Platforming/Animation",
            "Practice/Variant_Combat",
            "Practice/Variant_Combat/AI",
            "Practice/Variant_Combat/Animation",
            "Practice/Variant_Combat/Gameplay",
            "Practice/Variant_Combat/Interfaces",
            "Practice/Variant_Combat/LagComp",          // Week 07 — Lag Compensation
			"Practice/Variant_Combat/UI",
            "Practice/Variant_Combat/Week06",           // Week 06 — Team Deathmatch
            "Practice/Variant_Combat/Week08",           // Week 08 — Steam Online Sessions
            "Practice/Variant_Combat/Week04",           // Week 04 — Replicated Health & Damage
			"Practice/Variant_SideScrolling",
            "Practice/Variant_SideScrolling/AI",
            "Practice/Variant_SideScrolling/Gameplay",
            "Practice/Variant_SideScrolling/Interfaces",
            "Practice/Variant_SideScrolling/UI"
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
