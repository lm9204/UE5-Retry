// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Retry : ModuleRules
{
	public Retry(ReadOnlyTargetRules Target) : base(Target)
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
			"Retry",
			"Retry/Variant_Platforming",
			"Retry/Variant_Platforming/Animation",
			"Retry/Variant_Combat",
			"Retry/Variant_Combat/AI",
			"Retry/Variant_Combat/Animation",
			"Retry/Variant_Combat/Gameplay",
			"Retry/Variant_Combat/Interfaces",
			"Retry/Variant_Combat/UI",
			"Retry/Variant_SideScrolling",
			"Retry/Variant_SideScrolling/AI",
			"Retry/Variant_SideScrolling/Gameplay",
			"Retry/Variant_SideScrolling/Interfaces",
			"Retry/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
