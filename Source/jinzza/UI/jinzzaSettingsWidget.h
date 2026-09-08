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

/**
 * Full settings screen: Graphics / Audio / Controls / Gameplay tabs, backed by the real
 * UjinzzaGameUserSettings (see that class for why audio uses runtime SoundClass/SoundMix
 * objects and controls use per-action key overrides rather than content-authored assets).
 *
 * TEMP C++-built (see docs/umg_widget_authoring_guide.md): builds its own tree in
 * BuildWidgetTree() instead of relying on a Designer-authored WBP_Settings layout, so PIE isn't
 * blocked while the visual design pass hasn't happened yet - left tab sidebar (4 tab buttons +
 * 4 accent bars) next to a right content column holding TabSwitcher with one scrollable page
 * per tab, plus a bottom-right Apply/Back button pair. When that pass happens, delete
 * BuildWidgetTree(), restore `meta = (BindWidget)` on every property below, and lay them out
 * for real in WBP_Settings's Designer per the guide.
 */
UCLASS()
class JINZZA_API UjinzzaSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	FSimpleMulticastDelegate OnBackRequested;

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

private:
	void PopulateGraphicsPage();
	void PopulateAudioPage();
	void PopulateControlsPage();
	void PopulateGameplayPage();

	void StartRebind(FName ActionName);
	void RefreshRebindButtonLabel(FName ActionName);

	/** Switches the content page and moves the sidebar's active-tab accent bar. */
	void SetActiveTab(int32 TabIndex);

	void BuildWidgetTree();

	UPROPERTY()
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

	UPROPERTY()
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY()
	TObjectPtr<UButton> BackButton;

	// Sidebar tab buttons and their accent bars (shown only next to the active tab).
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

	UPROPERTY()
	TMap<FName, TObjectPtr<UTextBlock>> RebindLabels;

	bool bWaitingForRebind = false;
	FName PendingRebindAction;

	// Gameplay
	UPROPERTY() TObjectPtr<UCheckBox> SubtitlesCheckBox;
	UPROPERTY() TObjectPtr<UComboBoxString> ColorblindModeCombo;
	UPROPERTY() TObjectPtr<USlider> ColorblindStrengthSlider;
};
