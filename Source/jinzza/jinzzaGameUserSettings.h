// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameFramework/GameUserSettings.h"
#include "jinzzaGameUserSettings.generated.h"

class USoundClass;
class USoundMix;

/** Colorblind-friendly rendering mode, applied via Slate's built-in color vision deficiency correction. */
UENUM(BlueprintType)
enum class EJinzzaColorblindMode : uint8
{
	Off,
	Deuteranope,
	Protanope,
	Tritanope
};

/**
 * How the local player's mic is gated. Stored here as a plain preference - not consumed yet,
 * since UVoiceDisguiseComponent/UProximityVoiceComponent (design doc section 11) don't exist
 * yet, but per the reference brief's adaptation plan this needs to exist before those do: a
 * near-field voice game needs push-to-talk far more than a typical single-player-feel project.
 */
UENUM(BlueprintType)
enum class EJinzzaMicInputMode : uint8
{
	PushToTalk,
	OpenMic
};

/**
 * Project-specific persisted settings, extending the engine's real UGameUserSettings
 * (config-file backed, same base class every shipped UE game uses for display/graphics
 * options) with audio mix levels, accessibility, and control preferences. Registered as
 * the project's settings class via GameUserSettingsClassName in DefaultEngine.ini.
 *
 * Modeled on the settings architecture Epic's Lyra sample project uses: GameUserSettings
 * for display/graphics, real SoundClass/SoundMix objects for audio category mixing, and
 * per-player key remap overrides applied on top of the default Enhanced Input mapping
 * contexts (see AjinzzaPlayerController::SetupInputComponent).
 */
UCLASS()
class JINZZA_API UjinzzaGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UjinzzaGameUserSettings();

	/** Convenience accessor for the active settings object, cast to this project's subclass. */
	static UjinzzaGameUserSettings* Get();

	virtual void SetToDefaults() override;
	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

	// --- Audio ---
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float NewValue);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMusicVolume() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float NewValue);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetSFXVolume() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSFXVolume(float NewValue);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetVoiceVolume() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetVoiceVolume(float NewValue);

	/** Sound classes any audio content in the project should assign itself to, so category sliders affect it. */
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	USoundClass* GetMusicSoundClass() const;
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	USoundClass* GetSFXSoundClass() const;
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	USoundClass* GetVoiceSoundClass() const;

	/** Push-to-talk vs. open mic for proximity/disguised voice (see UVoiceDisguiseComponent, once it exists). */
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	EJinzzaMicInputMode GetMicInputMode() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMicInputMode(EJinzzaMicInputMode NewValue);

	/** Selected input device's platform id (from UAudioCaptureBlueprintLibrary::GetAvailableAudioInputDevices). Empty = system default. */
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	FString GetMicDeviceId() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMicDeviceId(const FString& NewDeviceId);

	// --- Gameplay / accessibility ---
	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	bool GetSubtitlesEnabled() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetSubtitlesEnabled(bool bNewValue);

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	EJinzzaColorblindMode GetColorblindMode() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetColorblindMode(EJinzzaColorblindMode NewValue);

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	float GetColorblindStrength() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetColorblindStrength(float NewValue);

	// --- Controls ---
	UFUNCTION(BlueprintPure, Category = "Settings|Controls")
	float GetMouseSensitivity() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetMouseSensitivity(float NewValue);

	UFUNCTION(BlueprintPure, Category = "Settings|Controls")
	bool GetInvertYAxis() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetInvertYAxis(bool bNewValue);

	/** Rebinds ActionName (an Input Action asset's FName) to Key. Pass EKeys::Invalid to clear the override. */
	void SetKeyRebind(FName ActionName, FKey Key);
	/** Returns the rebound key for ActionName, or EKeys::Invalid if it has no override. */
	FKey GetKeyRebind(FName ActionName) const;
	const TMap<FName, FKey>& GetAllKeyRebinds() const { return KeyRebinds; }

	/** Fired after ApplySettings so live gameplay (e.g. the active input mapping contexts) can react. */
	DECLARE_MULTICAST_DELEGATE(FOnJinzzaSettingsApplied);
	FOnJinzzaSettingsApplied OnSettingsApplied;

private:
	void ApplyAudioSettings();
	void ApplyAccessibilitySettings();
	void EnsureAudioObjects();

	UPROPERTY(config)
	float MasterVolume = 1.f;

	UPROPERTY(config)
	float MusicVolume = 0.8f;

	UPROPERTY(config)
	float SFXVolume = 1.f;

	UPROPERTY(config)
	float VoiceVolume = 1.f;

	UPROPERTY(config)
	EJinzzaMicInputMode MicInputMode = EJinzzaMicInputMode::PushToTalk;

	UPROPERTY(config)
	FString MicDeviceId;

	UPROPERTY(config)
	bool bSubtitlesEnabled = false;

	UPROPERTY(config)
	EJinzzaColorblindMode ColorblindMode = EJinzzaColorblindMode::Off;

	UPROPERTY(config)
	float ColorblindStrength = 1.f;

	UPROPERTY(config)
	float MouseSensitivity = 1.f;

	UPROPERTY(config)
	bool bInvertYAxis = false;

	UPROPERTY(config)
	TMap<FName, FKey> KeyRebinds;

	/** Runtime-only (not saved to disk) sound mix objects used to apply the volume sliders. Real SoundClass/SoundMix
	 *  objects, created transiently at runtime rather than as content assets, so the audio settings screen is fully
	 *  functional without requiring hand-authored .uasset audio content this project doesn't have yet. */
	UPROPERTY(Transient)
	TObjectPtr<USoundClass> MasterClass;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> MusicClass;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> SFXClass;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> VoiceClass;

	UPROPERTY(Transient)
	TObjectPtr<USoundMix> SettingsMix;
};
