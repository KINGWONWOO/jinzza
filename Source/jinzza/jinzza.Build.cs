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
			"AudioCapture",
			// USynthComponent::Start/Stop/CreateAudioComponent/GetAudioComponent (used by
			// UjinzzaVoiceTestWidget) are AUDIOMIXER_API - AudioCapture only re-exposes the
			// headers transitively, jinzza still needs AudioMixer directly to link against them.
			"AudioMixer",
			// USourceEffectRingModulationPreset/USourceEffectSimpleDelayPreset (robot/cave voice
			// DSP in UjinzzaVoiceTestWidget) live in the Synthesis plugin's own module - the plugin
			// was already enabled in jinzza.uproject, but linking against its classes needs this
			// too (same two-step gotcha as AudioCapture/AudioMixer above). NEW dependency - needs a
			// full UnrealBuildTool rebuild (editor closed), not just Live Coding.
			"Synthesis"
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
