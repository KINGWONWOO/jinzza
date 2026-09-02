// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class jinzza : ModuleRules
{
	public jinzza(ReadOnlyTargetRules Target) : base(Target)
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

		PrivateDependencyModuleNames.AddRange(new string[] {
			"SlateCore",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"AudioCapture"
		});

		PublicIncludePaths.AddRange(new string[] {
			"jinzza",
			"jinzza/UI",
			"jinzza/Variant_Horror",
			"jinzza/Variant_Horror/UI",
			"jinzza/Variant_Shooter",
			"jinzza/Variant_Shooter/AI",
			"jinzza/Variant_Shooter/UI",
			"jinzza/Variant_Shooter/Weapons"
		});

		// OnlineSubsystemSteam is enabled via the Plugins section of jinzza.uproject
		// (DefaultEngine.ini's [OnlineSubsystem]/[OnlineSubsystemSteam] sections configure it).
	}
}
