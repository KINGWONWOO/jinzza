// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AudioMixerBlueprintLibrary.h"
#include "jinzzaVoiceTestWidget.generated.h"

class UButton;
class UTextBlock;
class USlider;
class UjinzzaMicLoopbackComponent;
class UComboBoxString;
class USizeBox;
class USoundEffectSourcePresetChain;
class USourceEffectRingModulationPreset;
class USourceEffectSimpleDelayPreset;

/**
 * Voice modulation test panel: capture the mic (UjinzzaMicLoopbackComponent, real-time, local
 * loopback only - no networking) and play it back live through adjustable pitch/robot/echo DSP,
 * so the player can hear what a disguised voice sounds like before the real proximity-voice
 * system (EOS Voice Chat, design doc section 11, Week 6 - see [[eos-voice-chat-plan]]) exists.
 * Opened two places: via a button in the main menu (shown as a central UjinzzaMainMenuWidget
 * switcher page, same pattern as Settings/Customization), and popped up from
 * AjinzzaVoiceTestKiosk in the exhibition test level (as a viewport overlay) - both just
 * construct an instance, nothing kiosk-specific lives in this class. OnBackRequested fires on
 * Close; the main menu binds it to switch back to its buttons page, AjinzzaVoiceTestKiosk binds
 * it to remove this widget from the viewport - see UjinzzaSettingsWidget for the identical
 * pattern.
 *
 * Three sliders (Pitch/Robot/Echo) can be dragged directly, live - no Apply button needed. Three
 * template buttons (Cave/Helium/Robot) jump all three sliders to a preset combination in one
 * click. Pitch is UjinzzaMicLoopbackComponent's own real-time pitch shifter (SetPitch, live -
 * it used to be the audio component's resampling pitch multiplier, which changed playback speed
 * and needed a Stop/Start that cut the voice out). Robot (ring modulation) and Echo (a short
 * delay/feedback line, the "cave" part) are USourceEffectRingModulationPreset/
 * USourceEffectSimpleDelayPreset instances in a runtime-built SourceEffectChain on the loopback -
 * pushed live via SetSettings(). Robot/Echo/presets were toned down (2026-10-04) so the voice
 * stays understandable at full strength. Nothing here restarts the loopback.
 *
 * Devices (2026-10-04): a Mic row picks the capture device (Audio::FAudioCapture's list; saved to
 * UjinzzaGameUserSettings' MicDeviceId, the same setting the Settings screen edits) and a
 * Speaker row picks the game's audio output device (UAudioMixerBlueprintLibrary::
 * SwapAudioOutputDevice - applies to all game audio immediately, not saved across restarts). A
 * Mic Boost slider (1x-6x, default 3x) sets the loopback gain, and a live level meter under
 * Speak & Listen shows the mic is being heard - the old loopback played back at raw mic level,
 * which was hard to hear.
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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	FSimpleMulticastDelegate OnBackRequested;

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

	UFUNCTION()
	void OnBoostChanged(float NewValue);

	UFUNCTION()
	void OnMicDeviceSelected(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnOutputDeviceSelected(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnOutputDevicesObtained(const TArray<FAudioOutputDeviceInfo>& AvailableDevices);

	UFUNCTION()
	void OnOutputDeviceSwapped(const FSwapAudioOutputResult& SwapResult);

private:
	void BuildWidgetTree();
	void StopListening();
	void EnsureEffectChain();
	void ApplyPitch();
	void ApplyRobotSettings();
	void ApplyEchoSettings();
	void RefreshValueLabels();
	void PopulateMicDevices();
	void StartListening();
	/** Destroys the loopback component so the next StartListening opens the newly chosen mic. */
	void RecreateCapture();

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

	UPROPERTY()
	TObjectPtr<UComboBoxString> MicDeviceCombo;

	UPROPERTY()
	TObjectPtr<UComboBoxString> OutputDeviceCombo;

	UPROPERTY()
	TObjectPtr<USlider> BoostSlider;

	UPROPERTY()
	TObjectPtr<UTextBlock> BoostValueText;

	/** The level meter's fill bar - its width is set every tick from the loopback's input level. */
	UPROPERTY()
	TObjectPtr<USizeBox> LevelFill;

	/** Parallel to MicDeviceCombo's options: index 0 = system default (INDEX_NONE), then FAudioCapture device indices. */
	TArray<int32> MicDeviceIndices;
	TArray<FString> MicDeviceIds;
	int32 SelectedMicDeviceIndex = INDEX_NONE;

	/** Parallel to OutputDeviceCombo's options. */
	TArray<FString> OutputDeviceIds;

	/** Loopback gain (linear), 1-6. */
	float CurrentBoost = 3.f;

	/** Smoothed meter level (0-1). */
	float DisplayLevel = 0.f;

	/** Created on first Listen click, outered to the owning PlayerController so it survives
	 * independently of this widget - see NativeDestruct for cleanup. */
	UPROPERTY()
	TObjectPtr<UjinzzaMicLoopbackComponent> CaptureComponent;

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
