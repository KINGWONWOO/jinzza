// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaVoiceTestWidget.generated.h"

class UButton;
class UTextBlock;
class USlider;
class UAudioCaptureComponent;
class USoundEffectSourcePresetChain;
class USourceEffectRingModulationPreset;
class USourceEffectSimpleDelayPreset;

/**
 * Voice modulation test panel: capture the mic (UAudioCaptureComponent, real-time, local
 * loopback only - no networking) and play it back live through adjustable pitch/robot/echo DSP,
 * so the player can hear what a disguised voice sounds like before the real proximity-voice
 * system (EOS Voice Chat, design doc section 11, Week 6 - see [[eos-voice-chat-plan]]) exists.
 * Opened two places: embedded always-visible in the main menu's bottom-right corner, and popped
 * up from AjinzzaVoiceTestKiosk in the exhibition test level - both just construct an instance,
 * nothing kiosk-specific lives in this class.
 *
 * Three sliders (Pitch/Robot/Echo) can be dragged directly, live - no Apply button needed. Three
 * template buttons (Cave/Helium/Robot) jump all three sliders to a preset combination in one
 * click. Pitch uses UAudioComponent::SetPitchMultiplier (simple resampling pitch/speed shift -
 * the actual mechanism behind the classic "chipmunk"/helium and slowed-down/cave voice memes, no
 * DSP plugin needed). Robot (ring modulation) and Echo (a short delay/feedback line, the "cave"
 * reverb-ish part) are real USourceEffectRingModulationPreset/USourceEffectSimpleDelayPreset
 * instances living in a runtime-built SourceEffectChain assigned to the capture component -
 * their Settings are pushed live via SetSettings() while listening, no restart needed (only a
 * Pitch change needs a restart - see ApplyPitch).
 *
 * TEMP C++-built (see docs/umg_widget_authoring_guide.md, same pattern as every other widget in
 * this project): builds its own tree in BuildWidgetTree() since no Designer-authored
 * WBP_VoiceTest layout exists.
 */
UCLASS()
class JINZZA_API UjinzzaVoiceTestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	void OnToggleListenClicked();

	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnPitchChanged(float NewValue);

	UFUNCTION()
	void OnRobotChanged(float NewValue);

	UFUNCTION()
	void OnEchoChanged(float NewValue);

	UFUNCTION()
	void OnCaveClicked();

	UFUNCTION()
	void OnHeliumClicked();

	UFUNCTION()
	void OnRobotPresetClicked();

private:
	void BuildWidgetTree();
	void StopListening();
	void EnsureEffectChain();
	void ApplyPitch();
	void ApplyRobotSettings();
	void ApplyEchoSettings();
	void RefreshValueLabels();

	/** Sets all three sliders/values to Pitch/Robot/Echo and applies them - shared by the three template buttons. */
	void ApplyPreset(float NewPitch, float NewRobot, float NewEcho);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ToggleListenButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ToggleListenButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> PitchSlider;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PitchValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> RobotSlider;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RobotValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> EchoSlider;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EchoValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CaveButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> HeliumButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RobotPresetButton;

	/** Created on first Listen click, outered to the owning PlayerController so it survives
	 * independently of this widget - see NativeDestruct for cleanup. */
	UPROPERTY()
	TObjectPtr<UAudioCaptureComponent> CaptureComponent;

	/** Built once (EnsureEffectChain) and assigned to CaptureComponent->SourceEffectChain before
	 * the first Start() - always present in the chain, but silent (fully dry) until Robot/Echo
	 * sliders are raised above 0, so there's no audible difference from "no effects" at rest. */
	UPROPERTY()
	TObjectPtr<USoundEffectSourcePresetChain> EffectChain;

	UPROPERTY()
	TObjectPtr<USourceEffectRingModulationPreset> RingModPreset;

	UPROPERTY()
	TObjectPtr<USourceEffectSimpleDelayPreset> DelayPreset;

	/** 0.5 - 2.0, 1.0 = unmodified. */
	float CurrentPitch = 1.f;

	/** 0.0 - 1.0, 0 = no ring modulation. */
	float CurrentRobotAmount = 0.f;

	/** 0.0 - 1.0, 0 = no delay/echo. */
	float CurrentEchoAmount = 0.f;

	bool bIsListening = false;
};
