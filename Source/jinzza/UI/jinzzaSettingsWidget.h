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

/**
 * Full settings screen: Graphics / Audio / Controls / Gameplay tabs, backed by the real
 * UjinzzaGameUserSettings (see that class for why audio uses runtime SoundClass/SoundMix
 * objects and controls use per-action key overrides rather than content-authored assets).
 * Modeled on the settings architecture Epic's Lyra sample project uses.
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
	void BuildGraphicsPage(class UWidgetTree* Tree, class UPanelWidget* Parent);
	void BuildAudioPage(class UWidgetTree* Tree, class UPanelWidget* Parent);
	void BuildControlsPage(class UWidgetTree* Tree, class UPanelWidget* Parent);
	void BuildGameplayPage(class UWidgetTree* Tree, class UPanelWidget* Parent);

	UButton* AddRebindRow(class UWidgetTree* Tree, class UPanelWidget* Parent, FName ActionName, const FText& RowLabel);
	void StartRebind(FName ActionName);
	void RefreshRebindButtonLabel(FName ActionName);

	UPROPERTY()
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

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

	UPROPERTY()
	TMap<FName, TObjectPtr<UTextBlock>> RebindLabels;

	bool bWaitingForRebind = false;
	FName PendingRebindAction;

	// Gameplay
	UPROPERTY() TObjectPtr<UCheckBox> SubtitlesCheckBox;
	UPROPERTY() TObjectPtr<UComboBoxString> ColorblindModeCombo;
	UPROPERTY() TObjectPtr<USlider> ColorblindStrengthSlider;
};
