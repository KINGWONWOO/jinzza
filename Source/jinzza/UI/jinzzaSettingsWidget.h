// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AudioCaptureBlueprintLibrary.h"
#include "jinzzaSettingsWidget.generated.h"

class UWidgetSwitcher;
class UComboBoxString;
class USpinBox;
class USlider;
class UCheckBox;
class UButton;
class UTextBlock;
class UWidget;
class USizeBox;

class UjinzzaSettingsWidget;

/** Click target for one pill of a segmented selector in UjinzzaSettingsWidget (dynamic delegates need a UFUNCTION per bound object). */
UCLASS()
class UJinzzaSettingsSegmentHandler : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleClicked();

	TWeakObjectPtr<UjinzzaSettingsWidget> OwnerWidget;
	int32 GroupIndex = INDEX_NONE;
	int32 OptionIndex = INDEX_NONE;
};

/**
 * Full settings screen: Graphics / Audio / Controls / Gameplay tabs, backed by the real
 * UjinzzaGameUserSettings (see that class for why audio uses runtime SoundClass/SoundMix
 * objects and controls use per-action key overrides rather than content-authored assets).
 *
 * Layout - simple, big and cartoony to match T_Logo / the main menu: one big black sticker
 * panel (1240x780) with a 48pt "Settings" title and big tab pills (yellow dot under the active
 * one); per tab a short scrollable page whose sections are labeled with white speech bubbles
 * (the logo's "who is?" bubble); each row is a 24pt label left and its control right (580px).
 * Almost every choice is a row of big pills (20pt): window mode, quality preset, FPS cap,
 * Off/On toggles, mic mode, colorblind mode. Sliders are chunky with a yellow % readout; key
 * bindings show the key as a big key cap + "Change". Back/Apply (26/28pt) bottom-right.
 *
 * The pill rows are a view over the original data widgets: each group writes into a USpinBox /
 * UComboBoxString / UCheckBox (parked collapsed in HiddenHolders) that PopulateXxxPage /
 * OnApplyClicked already read and write, so the load/apply logic is unchanged. The single
 * Quality preset also writes all nine per-feature quality spin boxes, which no longer get rows.
 *
 * TEMP C++-built (see docs/umg_widget_authoring_guide.md): builds its own tree in
 * BuildWidgetTree() instead of a Designer-authored WBP_Settings layout. When that pass happens,
 * delete BuildWidgetTree(), restore `meta = (BindWidget)` on every property below, and lay them
 * out for real in WBP_Settings's Designer per the guide.
 */
UCLASS()
class JINZZA_API UjinzzaSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	FSimpleMulticastDelegate OnBackRequested;

	/** Shrinks the panel to a smaller 960x640 window for the in-game ESC menu, which shows it
	 * centered over the live game instead of the main menu's full-size page. Content scrolls as before. */
	void SetCompactLayout();

	/** Called by UJinzzaSettingsSegmentHandler: picks option OptionIndex of segmented group GroupIndex. */
	void SelectSegment(int32 GroupIndex, int32 OptionIndex);

protected:
	UFUNCTION()
	void OnBackClicked();

	UFUNCTION()
	void OnApplyClicked();

	UFUNCTION()
	void OnTabGraphicsClicked();
	UFUNCTION()
	void OnTabAudioClicked();
	UFUNCTION()
	void OnTabControlsClicked();
	UFUNCTION()
	void OnTabGameplayClicked();

	UFUNCTION()
	void OnAudioInputDevicesObtained(const TArray<FAudioInputDeviceInfo>& AvailableDevices);

	UFUNCTION()
	void OnRebindJumpClicked();
	UFUNCTION()
	void OnRebindSprintClicked();
	UFUNCTION()
	void OnRebindInteractClicked();
	UFUNCTION()
	void OnRebindPushToTalkClicked();

	/** Bound to every slider's OnValueChanged - refreshes the value readouts. */
	UFUNCTION()
	void HandleSliderValueChanged(float Value);

private:
	/** One segmented pill selector: the pills plus the data widget it writes into (exactly one of
	 * Spin/Combo/Check). A Spin writes the option index, or Values[option] when Values is set (e.g.
	 * frame-rate caps); a Check is a two-pill Off/On toggle. */
	struct FSegmentGroup
	{
		TWeakObjectPtr<USpinBox> Spin;
		TWeakObjectPtr<UComboBoxString> Combo;
		TWeakObjectPtr<UCheckBox> Check;
		TArray<float> Values;
		TArray<TWeakObjectPtr<UButton>> Buttons;
	};

	/** A slider's value readout and how to format it. */
	struct FSliderReadout
	{
		TWeakObjectPtr<USlider> Slider;
		TWeakObjectPtr<UTextBlock> Label;
		bool bPercent = true;
	};

	void RefreshSegments();
	void RefreshSliderReadouts();

	TArray<FSegmentGroup> SegmentGroups;
	int32 OverallQualityGroup = INDEX_NONE;
	TArray<int32> DetailQualityGroups;
	TArray<FSliderReadout> SliderReadouts;

	/** Keeps the segment click handlers alive (dynamic delegates only hold weak refs). */
	UPROPERTY()
	TArray<TObjectPtr<UJinzzaSettingsSegmentHandler>> SegmentHandlers;

	void PopulateGraphicsPage();
	void PopulateAudioPage();
	void PopulateControlsPage();
	void PopulateGameplayPage();

	void StartRebind(FName ActionName);
	void RefreshRebindButtonLabel(FName ActionName);

	/** Switches the content page, fills the active tab pill and shows its accent dot. */
	void SetActiveTab(int32 TabIndex);

	void BuildWidgetTree();

	/** Fixed-size box around the whole panel - resized by SetCompactLayout. */
	UPROPERTY()
	TObjectPtr<USizeBox> PanelBox;

	UPROPERTY()
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

	UPROPERTY()
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY()
	TObjectPtr<UButton> BackButton;

	// Header tab buttons and their accent dots (shown only under the active tab).
	UPROPERTY() TObjectPtr<UButton> GraphicsTabButton;
	UPROPERTY() TObjectPtr<UButton> AudioTabButton;
	UPROPERTY() TObjectPtr<UButton> ControlsTabButton;
	UPROPERTY() TObjectPtr<UButton> GameplayTabButton;
	UPROPERTY() TObjectPtr<UWidget> GraphicsTabAccent;
	UPROPERTY() TObjectPtr<UWidget> AudioTabAccent;
	UPROPERTY() TObjectPtr<UWidget> ControlsTabAccent;
	UPROPERTY() TObjectPtr<UWidget> GameplayTabAccent;

	// Graphics
	UPROPERTY() TObjectPtr<UComboBoxString> WindowModeCombo;
	UPROPERTY() TObjectPtr<UComboBoxString> ResolutionCombo;
	UPROPERTY() TObjectPtr<UCheckBox> VSyncCheckBox;
	UPROPERTY() TObjectPtr<USpinBox> FrameRateLimitSpinBox;
	UPROPERTY() TObjectPtr<UComboBoxString> OverallQualityCombo;
	UPROPERTY() TObjectPtr<USpinBox> ViewDistanceSpinBox;
	UPROPERTY() TObjectPtr<USpinBox> ShadowSpinBox;
	UPROPERTY() TObjectPtr<USpinBox> GlobalIlluminationSpinBox;
	UPROPERTY() TObjectPtr<USpinBox> ReflectionSpinBox;
	UPROPERTY() TObjectPtr<USpinBox> AntiAliasingSpinBox;
	UPROPERTY() TObjectPtr<USpinBox> TextureSpinBox;
	UPROPERTY() TObjectPtr<USpinBox> EffectsSpinBox;
	UPROPERTY() TObjectPtr<USpinBox> FoliageSpinBox;
	UPROPERTY() TObjectPtr<USpinBox> ShadingSpinBox;

	TArray<FIntPoint> AvailableResolutions;

	// Audio
	UPROPERTY() TObjectPtr<USlider> MasterVolumeSlider;
	UPROPERTY() TObjectPtr<USlider> MusicVolumeSlider;
	UPROPERTY() TObjectPtr<USlider> SFXVolumeSlider;
	UPROPERTY() TObjectPtr<USlider> VoiceVolumeSlider;
	UPROPERTY() TObjectPtr<UComboBoxString> MicInputModeCombo;
	UPROPERTY() TObjectPtr<UComboBoxString> MicDeviceCombo;
	/** Parallel to MicDeviceCombo's options - index 0 is always "" (system default). */
	TArray<FString> AvailableMicDeviceIds;

	// Controls
	UPROPERTY() TObjectPtr<USlider> MouseSensitivitySlider;
	UPROPERTY() TObjectPtr<UCheckBox> InvertYCheckBox;

	// Key rebind rows: one button (click to rebind) + one label (shows the current key) per action.
	// Shoot/SwapWeapon rows were removed (2026-09-08) - IA_Shoot/IA_SwapWeapon are Variant_Shooter
	// template leftovers AjinzzaCharacter never binds, so rebinding them did nothing (dead UI,
	// flagged 2026-09-07; this project has no weapon system per the design doc).
	UPROPERTY() TObjectPtr<UButton> JumpRebindButton;
	UPROPERTY() TObjectPtr<UTextBlock> JumpRebindLabel;
	UPROPERTY() TObjectPtr<UButton> SprintRebindButton;
	UPROPERTY() TObjectPtr<UTextBlock> SprintRebindLabel;
	UPROPERTY() TObjectPtr<UButton> InteractRebindButton;
	UPROPERTY() TObjectPtr<UTextBlock> InteractRebindLabel;
	UPROPERTY() TObjectPtr<UButton> PushToTalkRebindButton;
	UPROPERTY() TObjectPtr<UTextBlock> PushToTalkRebindLabel;

	UPROPERTY()
	TMap<FName, TObjectPtr<UTextBlock>> RebindLabels;

	bool bWaitingForRebind = false;
	FName PendingRebindAction;

	// Gameplay
	UPROPERTY() TObjectPtr<UCheckBox> SubtitlesCheckBox;
	UPROPERTY() TObjectPtr<UComboBoxString> ColorblindModeCombo;
	UPROPERTY() TObjectPtr<USlider> ColorblindStrengthSlider;
};
