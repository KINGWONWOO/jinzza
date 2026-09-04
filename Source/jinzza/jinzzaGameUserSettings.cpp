// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGameUserSettings.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace
{
	/** Settings objects have no world of their own; find whichever game/PIE world is currently running. */
	UWorld* FindActiveGameWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE))
			{
				return Context.World();
			}
		}
		return nullptr;
	}
}

UjinzzaGameUserSettings::UjinzzaGameUserSettings()
{
}

UjinzzaGameUserSettings* UjinzzaGameUserSettings::Get()
{
	return GEngine ? Cast<UjinzzaGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void UjinzzaGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	MasterVolume = 1.f;
	MusicVolume = 0.8f;
	SFXVolume = 1.f;
	VoiceVolume = 1.f;
	bSubtitlesEnabled = false;
	ColorblindMode = EJinzzaColorblindMode::Off;
	ColorblindStrength = 1.f;
	MouseSensitivity = 1.f;
	bInvertYAxis = false;
	KeyRebinds.Empty();
	HeadStyle = EJinzzaCustomizationStyle::StyleA;
	HairColor = EJinzzaHairColor::Black;
	TopStyle = EJinzzaCustomizationStyle::StyleA;
	EyebrowsStyle = EJinzzaCustomizationStyle::StyleA;
	EyesStyle = EJinzzaCustomizationStyle::StyleA;
}

void UjinzzaGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	Super::ApplySettings(bCheckForCommandLineOverrides);

	ApplyAudioSettings();
	ApplyAccessibilitySettings();

	OnSettingsApplied.Broadcast();
}

float UjinzzaGameUserSettings::GetMasterVolume() const { return MasterVolume; }
void UjinzzaGameUserSettings::SetMasterVolume(float NewValue) { MasterVolume = FMath::Clamp(NewValue, 0.f, 1.f); }

float UjinzzaGameUserSettings::GetMusicVolume() const { return MusicVolume; }
void UjinzzaGameUserSettings::SetMusicVolume(float NewValue) { MusicVolume = FMath::Clamp(NewValue, 0.f, 1.f); }

float UjinzzaGameUserSettings::GetSFXVolume() const { return SFXVolume; }
void UjinzzaGameUserSettings::SetSFXVolume(float NewValue) { SFXVolume = FMath::Clamp(NewValue, 0.f, 1.f); }

float UjinzzaGameUserSettings::GetVoiceVolume() const { return VoiceVolume; }
void UjinzzaGameUserSettings::SetVoiceVolume(float NewValue) { VoiceVolume = FMath::Clamp(NewValue, 0.f, 1.f); }

USoundClass* UjinzzaGameUserSettings::GetMusicSoundClass() const { return MusicClass; }
USoundClass* UjinzzaGameUserSettings::GetSFXSoundClass() const { return SFXClass; }
USoundClass* UjinzzaGameUserSettings::GetVoiceSoundClass() const { return VoiceClass; }

EJinzzaMicInputMode UjinzzaGameUserSettings::GetMicInputMode() const { return MicInputMode; }
void UjinzzaGameUserSettings::SetMicInputMode(EJinzzaMicInputMode NewValue) { MicInputMode = NewValue; }

FString UjinzzaGameUserSettings::GetMicDeviceId() const { return MicDeviceId; }
void UjinzzaGameUserSettings::SetMicDeviceId(const FString& NewDeviceId) { MicDeviceId = NewDeviceId; }

EJinzzaCustomizationStyle UjinzzaGameUserSettings::GetHeadStyle() const { return HeadStyle; }
void UjinzzaGameUserSettings::SetHeadStyle(EJinzzaCustomizationStyle NewValue) { HeadStyle = NewValue; }

EJinzzaHairColor UjinzzaGameUserSettings::GetHairColor() const { return HairColor; }
void UjinzzaGameUserSettings::SetHairColor(EJinzzaHairColor NewValue) { HairColor = NewValue; }

EJinzzaCustomizationStyle UjinzzaGameUserSettings::GetTopStyle() const { return TopStyle; }
void UjinzzaGameUserSettings::SetTopStyle(EJinzzaCustomizationStyle NewValue) { TopStyle = NewValue; }

EJinzzaCustomizationStyle UjinzzaGameUserSettings::GetEyebrowsStyle() const { return EyebrowsStyle; }
void UjinzzaGameUserSettings::SetEyebrowsStyle(EJinzzaCustomizationStyle NewValue) { EyebrowsStyle = NewValue; }

EJinzzaCustomizationStyle UjinzzaGameUserSettings::GetEyesStyle() const { return EyesStyle; }
void UjinzzaGameUserSettings::SetEyesStyle(EJinzzaCustomizationStyle NewValue) { EyesStyle = NewValue; }

bool UjinzzaGameUserSettings::GetSubtitlesEnabled() const { return bSubtitlesEnabled; }
void UjinzzaGameUserSettings::SetSubtitlesEnabled(bool bNewValue) { bSubtitlesEnabled = bNewValue; }

EJinzzaColorblindMode UjinzzaGameUserSettings::GetColorblindMode() const { return ColorblindMode; }
void UjinzzaGameUserSettings::SetColorblindMode(EJinzzaColorblindMode NewValue) { ColorblindMode = NewValue; }

float UjinzzaGameUserSettings::GetColorblindStrength() const { return ColorblindStrength; }
void UjinzzaGameUserSettings::SetColorblindStrength(float NewValue) { ColorblindStrength = FMath::Clamp(NewValue, 0.f, 1.f); }

float UjinzzaGameUserSettings::GetMouseSensitivity() const { return MouseSensitivity; }
void UjinzzaGameUserSettings::SetMouseSensitivity(float NewValue) { MouseSensitivity = FMath::Clamp(NewValue, 0.1f, 5.f); }

bool UjinzzaGameUserSettings::GetInvertYAxis() const { return bInvertYAxis; }
void UjinzzaGameUserSettings::SetInvertYAxis(bool bNewValue) { bInvertYAxis = bNewValue; }

void UjinzzaGameUserSettings::EnsureAudioObjects()
{
	if (MasterClass)
	{
		return;
	}

	MasterClass = NewObject<USoundClass>(this, TEXT("Jinzza_MasterClass"));
	MusicClass = NewObject<USoundClass>(this, TEXT("Jinzza_MusicClass"));
	SFXClass = NewObject<USoundClass>(this, TEXT("Jinzza_SFXClass"));
	VoiceClass = NewObject<USoundClass>(this, TEXT("Jinzza_VoiceClass"));

	MusicClass->ParentClass = MasterClass;
	SFXClass->ParentClass = MasterClass;
	VoiceClass->ParentClass = MasterClass;
	MasterClass->ChildClasses = { MusicClass, SFXClass, VoiceClass };

	SettingsMix = NewObject<USoundMix>(this, TEXT("Jinzza_SettingsMix"));
}

void UjinzzaGameUserSettings::ApplyAudioSettings()
{
	EnsureAudioObjects();

	UWorld* World = FindActiveGameWorld();
	if (!World)
	{
		// No game world running yet (e.g. settings applied before any level loads) - the next
		// ApplySettings call once a world exists (main menu counts) will pick these values up.
		return;
	}

	UGameplayStatics::SetBaseSoundMix(World, SettingsMix);
	UGameplayStatics::SetSoundMixClassOverride(World, SettingsMix, MasterClass, MasterVolume, 1.f, 0.f, true);
	UGameplayStatics::SetSoundMixClassOverride(World, SettingsMix, MusicClass, MusicVolume, 1.f, 0.f, true);
	UGameplayStatics::SetSoundMixClassOverride(World, SettingsMix, SFXClass, SFXVolume, 1.f, 0.f, true);
	UGameplayStatics::SetSoundMixClassOverride(World, SettingsMix, VoiceClass, VoiceVolume, 1.f, 0.f, true);
}

void UjinzzaGameUserSettings::ApplyAccessibilitySettings()
{
	EColorVisionDeficiency Deficiency = EColorVisionDeficiency::NormalVision;
	switch (ColorblindMode)
	{
	case EJinzzaColorblindMode::Deuteranope: Deficiency = EColorVisionDeficiency::Deuteranope; break;
	case EJinzzaColorblindMode::Protanope: Deficiency = EColorVisionDeficiency::Protanope; break;
	case EJinzzaColorblindMode::Tritanope: Deficiency = EColorVisionDeficiency::Tritanope; break;
	case EJinzzaColorblindMode::Off:
	default: break;
	}

	UWidgetBlueprintLibrary::SetColorVisionDeficiencyType(Deficiency, ColorblindStrength, /*CorrectDeficiency=*/true, /*ShowCorrectionWithDeficiency=*/false);
}

void UjinzzaGameUserSettings::SetKeyRebind(FName ActionName, FKey Key)
{
	if (!Key.IsValid())
	{
		KeyRebinds.Remove(ActionName);
	}
	else
	{
		KeyRebinds.Add(ActionName, Key);
	}
}

FKey UjinzzaGameUserSettings::GetKeyRebind(FName ActionName) const
{
	if (const FKey* Found = KeyRebinds.Find(ActionName))
	{
		return *Found;
	}
	return EKeys::Invalid;
}
