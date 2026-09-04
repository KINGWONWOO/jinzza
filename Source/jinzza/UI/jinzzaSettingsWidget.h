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
 * UMG-authored: every property below must exist in this class's Widget Blueprint (e.g.
 * WBP_Settings), named exactly as below, for BindWidget to find it. Section headings and
 * per-row labels are purely decorative and don't need to be bound. Layout is expected to be
 * a left tab sidebar (4 tab buttons + 4 accent bars, one visible at a time) next to a right
 * content column holding a TabSwitcher with one scrollable page per tab, plus a bottom-right
 * Apply/Back button pair.
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
	void OnRebindShootClicked();
	UFUNCTION()
	void OnRebindSwapWeaponClicked();
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

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

	// Sidebar tab buttons and their accent bars (shown only next to the active tab).
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> GraphicsTabButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> AudioTabButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ControlsTabButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> GameplayTabButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> GraphicsTabAccent;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> AudioTabAccent;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> ControlsTabAccent;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> GameplayTabAccent;

	// Graphics
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UComboBoxString> WindowModeCombo;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UComboBoxString> ResolutionCombo;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCheckBox> VSyncCheckBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> FrameRateLimitSpinBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UComboBoxString> OverallQualityCombo;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> ViewDistanceSpinBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> ShadowSpinBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> GlobalIlluminationSpinBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> ReflectionSpinBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> AntiAliasingSpinBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> TextureSpinBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> EffectsSpinBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> FoliageSpinBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> ShadingSpinBox;

	TArray<FIntPoint> AvailableResolutions;

	// Audio
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> MasterVolumeSlider;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> MusicVolumeSlider;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> SFXVolumeSlider;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> VoiceVolumeSlider;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UComboBoxString> MicInputModeCombo;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UComboBoxString> MicDeviceCombo;
	/** Parallel to MicDeviceCombo's options - index 0 is always "" (system default). */
	TArray<FString> AvailableMicDeviceIds;

	// Controls
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> MouseSensitivitySlider;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCheckBox> InvertYCheckBox;

	// Key rebind rows: one button (click to rebind) + one label (shows the current key) per action.
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> JumpRebindButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> JumpRebindLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ShootRebindButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ShootRebindLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SwapWeaponRebindButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SwapWeaponRebindLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SprintRebindButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SprintRebindLabel;

	UPROPERTY()
	TMap<FName, TObjectPtr<UTextBlock>> RebindLabels;

	bool bWaitingForRebind = false;
	FName PendingRebindAction;

	// Gameplay
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCheckBox> SubtitlesCheckBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UComboBoxString> ColorblindModeCombo;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> ColorblindStrengthSlider;
};
