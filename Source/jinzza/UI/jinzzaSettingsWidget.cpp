// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaSettingsWidget.h"
#include "jinzzaGameUserSettings.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Widget.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"
#include "AudioCaptureBlueprintLibrary.h"

namespace
{
	enum ETabPage : int32
	{
		Tab_Graphics = 0,
		Tab_Audio = 1,
		Tab_Controls = 2,
		Tab_Gameplay = 3,
	};

	FString ResolutionToString(const FIntPoint& Res)
	{
		return FString::Printf(TEXT("%d x %d"), Res.X, Res.Y);
	}
}

void UjinzzaSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (GraphicsTabButton) GraphicsTabButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabGraphicsClicked);
	if (AudioTabButton) AudioTabButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabAudioClicked);
	if (ControlsTabButton) ControlsTabButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabControlsClicked);
	if (GameplayTabButton) GameplayTabButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabGameplayClicked);

	PopulateGraphicsPage();
	PopulateAudioPage();
	PopulateControlsPage();
	PopulateGameplayPage();

	SetActiveTab(Tab_Graphics);

	if (ApplyButton) ApplyButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnApplyClicked);
	if (BackButton) BackButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnBackClicked);

	if (JumpRebindButton) JumpRebindButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindJumpClicked);
	if (ShootRebindButton) ShootRebindButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindShootClicked);
	if (SwapWeaponRebindButton) SwapWeaponRebindButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindSwapWeaponClicked);
	if (SprintRebindButton) SprintRebindButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindSprintClicked);
}

void UjinzzaSettingsWidget::PopulateGraphicsPage()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (WindowModeCombo)
	{
		WindowModeCombo->AddOption(TEXT("Fullscreen"));
		WindowModeCombo->AddOption(TEXT("Windowed Fullscreen"));
		WindowModeCombo->AddOption(TEXT("Windowed"));

		FString CurrentWindowMode = TEXT("Fullscreen");
		switch (Settings->GetFullscreenMode())
		{
		case EWindowMode::Fullscreen: CurrentWindowMode = TEXT("Fullscreen"); break;
		case EWindowMode::WindowedFullscreen: CurrentWindowMode = TEXT("Windowed Fullscreen"); break;
		case EWindowMode::Windowed: CurrentWindowMode = TEXT("Windowed"); break;
		default: break;
		}
		WindowModeCombo->SetSelectedOption(CurrentWindowMode);
	}

	if (ResolutionCombo)
	{
		UKismetSystemLibrary::GetConvenientWindowedResolutions(AvailableResolutions);
		const FIntPoint CurrentResolution = Settings->GetScreenResolution();
		if (!AvailableResolutions.Contains(CurrentResolution))
		{
			AvailableResolutions.Insert(CurrentResolution, 0);
		}
		for (const FIntPoint& Res : AvailableResolutions)
		{
			ResolutionCombo->AddOption(ResolutionToString(Res));
		}
		ResolutionCombo->SetSelectedOption(ResolutionToString(CurrentResolution));
	}

	if (VSyncCheckBox)
	{
		VSyncCheckBox->SetIsChecked(Settings->IsVSyncEnabled());
	}

	if (FrameRateLimitSpinBox)
	{
		FrameRateLimitSpinBox->SetMinValue(0.f);
		FrameRateLimitSpinBox->SetMaxValue(300.f);
		FrameRateLimitSpinBox->SetMinSliderValue(0.f);
		FrameRateLimitSpinBox->SetMaxSliderValue(300.f);
		FrameRateLimitSpinBox->SetDelta(1.f);
		FrameRateLimitSpinBox->SetValue(Settings->GetFrameRateLimit());
	}

	if (OverallQualityCombo)
	{
		const TArray<FString> QualityPresets = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic") };
		for (const FString& Preset : QualityPresets)
		{
			OverallQualityCombo->AddOption(Preset);
		}
		const int32 OverallLevel = FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, QualityPresets.Num() - 1);
		OverallQualityCombo->SetSelectedOption(QualityPresets[OverallLevel]);
	}

	auto InitQualitySpinBox = [](USpinBox* SpinBox, int32 InitialValue)
	{
		if (!SpinBox)
		{
			return;
		}
		SpinBox->SetMinValue(0.f);
		SpinBox->SetMaxValue(4.f);
		SpinBox->SetMinSliderValue(0.f);
		SpinBox->SetMaxSliderValue(4.f);
		SpinBox->SetDelta(1.f);
		SpinBox->SetValue(static_cast<float>(InitialValue));
	};

	InitQualitySpinBox(ViewDistanceSpinBox, Settings->GetViewDistanceQuality());
	InitQualitySpinBox(ShadowSpinBox, Settings->GetShadowQuality());
	InitQualitySpinBox(GlobalIlluminationSpinBox, Settings->GetGlobalIlluminationQuality());
	InitQualitySpinBox(ReflectionSpinBox, Settings->GetReflectionQuality());
	InitQualitySpinBox(AntiAliasingSpinBox, Settings->GetAntiAliasingQuality());
	InitQualitySpinBox(TextureSpinBox, Settings->GetTextureQuality());
	InitQualitySpinBox(EffectsSpinBox, Settings->GetVisualEffectQuality());
	InitQualitySpinBox(FoliageSpinBox, Settings->GetFoliageQuality());
	InitQualitySpinBox(ShadingSpinBox, Settings->GetShadingQuality());
}

void UjinzzaSettingsWidget::PopulateAudioPage()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetMinValue(0.f);
		MasterVolumeSlider->SetMaxValue(1.f);
		MasterVolumeSlider->SetValue(Settings->GetMasterVolume());
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->SetMinValue(0.f);
		MusicVolumeSlider->SetMaxValue(1.f);
		MusicVolumeSlider->SetValue(Settings->GetMusicVolume());
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->SetMinValue(0.f);
		SFXVolumeSlider->SetMaxValue(1.f);
		SFXVolumeSlider->SetValue(Settings->GetSFXVolume());
	}
	if (VoiceVolumeSlider)
	{
		VoiceVolumeSlider->SetMinValue(0.f);
		VoiceVolumeSlider->SetMaxValue(1.f);
		VoiceVolumeSlider->SetValue(Settings->GetVoiceVolume());
	}

	if (MicInputModeCombo)
	{
		MicInputModeCombo->AddOption(TEXT("Push to Talk"));
		MicInputModeCombo->AddOption(TEXT("Open Mic"));
		const TArray<FString> MicModes = { TEXT("Push to Talk"), TEXT("Open Mic") };
		MicInputModeCombo->SetSelectedOption(MicModes[static_cast<int32>(Settings->GetMicInputMode())]);
	}

	if (MicDeviceCombo)
	{
		AvailableMicDeviceIds = { FString() };
		MicDeviceCombo->AddOption(TEXT("System Default"));
		MicDeviceCombo->SetSelectedOption(TEXT("System Default"));

		// Device list is fetched async; OnAudioInputDevicesObtained repopulates the combo (and
		// restores the saved selection) once the platform responds, usually within a frame or two.
		FOnAudioInputDevicesObtained DevicesObtained;
		DevicesObtained.BindDynamic(this, &UjinzzaSettingsWidget::OnAudioInputDevicesObtained);
		UAudioCaptureBlueprintLibrary::GetAvailableAudioInputDevices(this, DevicesObtained);
	}
}

void UjinzzaSettingsWidget::OnAudioInputDevicesObtained(const TArray<FAudioInputDeviceInfo>& AvailableDevices)
{
	if (!MicDeviceCombo)
	{
		return;
	}

	const UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	const FString SavedDeviceId = Settings ? Settings->GetMicDeviceId() : FString();

	MicDeviceCombo->ClearOptions();
	MicDeviceCombo->AddOption(TEXT("System Default"));
	AvailableMicDeviceIds = { FString() };

	for (const FAudioInputDeviceInfo& Device : AvailableDevices)
	{
		AvailableMicDeviceIds.Add(Device.DeviceId);
		MicDeviceCombo->AddOption(Device.DeviceName);
	}

	const int32 SavedIndex = AvailableMicDeviceIds.IndexOfByKey(SavedDeviceId);
	MicDeviceCombo->SetSelectedIndex(SavedIndex != INDEX_NONE ? SavedIndex : 0);
}

void UjinzzaSettingsWidget::PopulateControlsPage()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->SetMinValue(0.1f);
		MouseSensitivitySlider->SetMaxValue(3.f);
		MouseSensitivitySlider->SetValue(Settings->GetMouseSensitivity());
	}
	if (InvertYCheckBox)
	{
		InvertYCheckBox->SetIsChecked(Settings->GetInvertYAxis());
	}

	RebindLabels.Reset();
	RebindLabels.Add(TEXT("IA_Jump"), JumpRebindLabel);
	RebindLabels.Add(TEXT("IA_Shoot"), ShootRebindLabel);
	RebindLabels.Add(TEXT("IA_SwapWeapon"), SwapWeaponRebindLabel);
	RebindLabels.Add(TEXT("IA_Sprint"), SprintRebindLabel);

	RefreshRebindButtonLabel(TEXT("IA_Jump"));
	RefreshRebindButtonLabel(TEXT("IA_Shoot"));
	RefreshRebindButtonLabel(TEXT("IA_SwapWeapon"));
	RefreshRebindButtonLabel(TEXT("IA_Sprint"));
}

void UjinzzaSettingsWidget::PopulateGameplayPage()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (SubtitlesCheckBox)
	{
		SubtitlesCheckBox->SetIsChecked(Settings->GetSubtitlesEnabled());
	}

	if (ColorblindModeCombo)
	{
		const TArray<FString> ColorblindOptions = { TEXT("Off"), TEXT("Deuteranope"), TEXT("Protanope"), TEXT("Tritanope") };
		for (const FString& Option : ColorblindOptions)
		{
			ColorblindModeCombo->AddOption(Option);
		}
		ColorblindModeCombo->SetSelectedOption(ColorblindOptions[static_cast<int32>(Settings->GetColorblindMode())]);
	}

	if (ColorblindStrengthSlider)
	{
		ColorblindStrengthSlider->SetMinValue(0.f);
		ColorblindStrengthSlider->SetMaxValue(1.f);
		ColorblindStrengthSlider->SetValue(Settings->GetColorblindStrength());
	}
}

void UjinzzaSettingsWidget::StartRebind(FName ActionName)
{
	bWaitingForRebind = true;
	PendingRebindAction = ActionName;

	if (TObjectPtr<UTextBlock>* Label = RebindLabels.Find(ActionName))
	{
		if (*Label)
		{
			(*Label)->SetText(FText::FromString(TEXT("Press a key...")));
		}
	}

	SetKeyboardFocus();
}

void UjinzzaSettingsWidget::RefreshRebindButtonLabel(FName ActionName)
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (TObjectPtr<UTextBlock>* Label = RebindLabels.Find(ActionName))
	{
		if (*Label)
		{
			const FKey Key = Settings->GetKeyRebind(ActionName);
			(*Label)->SetText(FText::FromString(Key.IsValid() ? Key.GetDisplayName().ToString() : TEXT("Default")));
		}
	}
}

FReply UjinzzaSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bWaitingForRebind)
	{
		if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
		{
			Settings->SetKeyRebind(PendingRebindAction, InKeyEvent.GetKey());
		}

		RefreshRebindButtonLabel(PendingRebindAction);
		bWaitingForRebind = false;
		PendingRebindAction = NAME_None;
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UjinzzaSettingsWidget::OnRebindJumpClicked() { StartRebind(TEXT("IA_Jump")); }
void UjinzzaSettingsWidget::OnRebindShootClicked() { StartRebind(TEXT("IA_Shoot")); }
void UjinzzaSettingsWidget::OnRebindSwapWeaponClicked() { StartRebind(TEXT("IA_SwapWeapon")); }
void UjinzzaSettingsWidget::OnRebindSprintClicked() { StartRebind(TEXT("IA_Sprint")); }

void UjinzzaSettingsWidget::SetActiveTab(int32 TabIndex)
{
	if (TabSwitcher)
	{
		TabSwitcher->SetActiveWidgetIndex(TabIndex);
	}

	UWidget* Accents[] = { GraphicsTabAccent, AudioTabAccent, ControlsTabAccent, GameplayTabAccent };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Accents); ++Index)
	{
		if (Accents[Index])
		{
			Accents[Index]->SetVisibility(Index == TabIndex ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}
	}
}

void UjinzzaSettingsWidget::OnTabGraphicsClicked() { SetActiveTab(Tab_Graphics); }
void UjinzzaSettingsWidget::OnTabAudioClicked() { SetActiveTab(Tab_Audio); }
void UjinzzaSettingsWidget::OnTabControlsClicked() { SetActiveTab(Tab_Controls); }
void UjinzzaSettingsWidget::OnTabGameplayClicked() { SetActiveTab(Tab_Gameplay); }

void UjinzzaSettingsWidget::OnBackClicked()
{
	OnBackRequested.Broadcast();
}

void UjinzzaSettingsWidget::OnApplyClicked()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	// Graphics
	if (WindowModeCombo)
	{
		const FString Selection = WindowModeCombo->GetSelectedOption();
		EWindowMode::Type Mode = EWindowMode::Fullscreen;
		if (Selection == TEXT("Windowed Fullscreen")) Mode = EWindowMode::WindowedFullscreen;
		else if (Selection == TEXT("Windowed")) Mode = EWindowMode::Windowed;
		Settings->SetFullscreenMode(Mode);
	}
	if (ResolutionCombo)
	{
		const int32 Index = ResolutionCombo->GetSelectedIndex();
		if (AvailableResolutions.IsValidIndex(Index))
		{
			Settings->SetScreenResolution(AvailableResolutions[Index]);
		}
	}
	if (VSyncCheckBox) Settings->SetVSyncEnabled(VSyncCheckBox->IsChecked());
	if (FrameRateLimitSpinBox) Settings->SetFrameRateLimit(FrameRateLimitSpinBox->GetValue());
	if (OverallQualityCombo) Settings->SetOverallScalabilityLevel(OverallQualityCombo->GetSelectedIndex());
	if (ViewDistanceSpinBox) Settings->SetViewDistanceQuality(FMath::RoundToInt(ViewDistanceSpinBox->GetValue()));
	if (ShadowSpinBox) Settings->SetShadowQuality(FMath::RoundToInt(ShadowSpinBox->GetValue()));
	if (GlobalIlluminationSpinBox) Settings->SetGlobalIlluminationQuality(FMath::RoundToInt(GlobalIlluminationSpinBox->GetValue()));
	if (ReflectionSpinBox) Settings->SetReflectionQuality(FMath::RoundToInt(ReflectionSpinBox->GetValue()));
	if (AntiAliasingSpinBox) Settings->SetAntiAliasingQuality(FMath::RoundToInt(AntiAliasingSpinBox->GetValue()));
	if (TextureSpinBox) Settings->SetTextureQuality(FMath::RoundToInt(TextureSpinBox->GetValue()));
	if (EffectsSpinBox) Settings->SetVisualEffectQuality(FMath::RoundToInt(EffectsSpinBox->GetValue()));
	if (FoliageSpinBox) Settings->SetFoliageQuality(FMath::RoundToInt(FoliageSpinBox->GetValue()));
	if (ShadingSpinBox) Settings->SetShadingQuality(FMath::RoundToInt(ShadingSpinBox->GetValue()));

	// Audio
	if (MasterVolumeSlider) Settings->SetMasterVolume(MasterVolumeSlider->GetValue());
	if (MusicVolumeSlider) Settings->SetMusicVolume(MusicVolumeSlider->GetValue());
	if (SFXVolumeSlider) Settings->SetSFXVolume(SFXVolumeSlider->GetValue());
	if (VoiceVolumeSlider) Settings->SetVoiceVolume(VoiceVolumeSlider->GetValue());
	if (MicInputModeCombo) Settings->SetMicInputMode(MicInputModeCombo->GetSelectedIndex() == 1 ? EJinzzaMicInputMode::OpenMic : EJinzzaMicInputMode::PushToTalk);
	if (MicDeviceCombo && AvailableMicDeviceIds.IsValidIndex(MicDeviceCombo->GetSelectedIndex()))
	{
		Settings->SetMicDeviceId(AvailableMicDeviceIds[MicDeviceCombo->GetSelectedIndex()]);
	}

	// Controls
	if (MouseSensitivitySlider) Settings->SetMouseSensitivity(MouseSensitivitySlider->GetValue());
	if (InvertYCheckBox) Settings->SetInvertYAxis(InvertYCheckBox->IsChecked());

	// Gameplay
	if (SubtitlesCheckBox) Settings->SetSubtitlesEnabled(SubtitlesCheckBox->IsChecked());
	if (ColorblindModeCombo) Settings->SetColorblindMode(static_cast<EJinzzaColorblindMode>(ColorblindModeCombo->GetSelectedIndex()));
	if (ColorblindStrengthSlider) Settings->SetColorblindStrength(ColorblindStrengthSlider->GetValue());

	Settings->ApplySettings(false);
	Settings->SaveSettings();
}
