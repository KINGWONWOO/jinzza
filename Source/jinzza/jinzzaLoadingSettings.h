// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "jinzzaLoadingSettings.generated.h"

/** Extra assets to load behind the loading screen when arriving in one level. */
USTRUCT()
struct FJinzzaMapPreloadList
{
	GENERATED_BODY()

	/** Short map name, e.g. "Lvl_Lobby". */
	UPROPERTY(EditAnywhere, Category = "Loading")
	FString MapName;

	UPROPERTY(EditAnywhere, Category = "Loading", meta = (AllowedClasses = "/Script/CoreUObject.Object"))
	TArray<FSoftObjectPath> Assets;
};

/**
 * Project Settings > Game > JINZZA Loading. Drives UjinzzaLoadingScreenSubsystem and the
 * all-players-ready gates (AjinzzaLobbyGameMode match preparation, AjinzzaGameGameMode match start).
 *
 * The preload lists are things loaded lazily by code (LoadObject at first use: BGM, UI sounds, prop
 * meshes, fonts...), which would otherwise hitch the first time they're needed. Assets placed in a
 * level load with the level anyway. Defaults are set in the constructor; edit them in Project Settings.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "JINZZA Loading"))
class JINZZA_API UjinzzaLoadingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UjinzzaLoadingSettings();

	static const UjinzzaLoadingSettings* Get() { return GetDefault<UjinzzaLoadingSettings>(); }

	/** Assets returned by the preload lists for MapName (common + that map's own). */
	TArray<FSoftObjectPath> GetPreloadAssetsForMap(const FString& MapName) const;

	/** Loaded on every level arrival (UI art, fonts, UI sounds, input). */
	UPROPERTY(config, EditAnywhere, Category = "Preload", meta = (AllowedClasses = "/Script/CoreUObject.Object"))
	TArray<FSoftObjectPath> CommonPreloadAssets;

	UPROPERTY(config, EditAnywhere, Category = "Preload")
	TArray<FJinzzaMapPreloadList> MapPreloadAssets;

	/** One is picked at random each time the loading screen shows. */
	UPROPERTY(config, EditAnywhere, Category = "Display")
	TArray<FText> Tips;

	/** The screen stays up at least this long so a fast load doesn't just flash. */
	UPROPERTY(config, EditAnywhere, Category = "Display", meta = (ClampMin = "0"))
	float MinimumDisplaySeconds = 0.75f;

	/** Gives up waiting (hides the screen anyway) after this long - safety net for a stuck load. */
	UPROPERTY(config, EditAnywhere, Category = "Display", meta = (ClampMin = "10"))
	float MaximumDisplaySeconds = 120.f;

	/** Lobby -> match: the host travels once every player has preloaded the match, or after this many seconds. */
	UPROPERTY(config, EditAnywhere, Category = "Match", meta = (ClampMin = "1"))
	float MatchPreloadTimeoutSeconds = 30.f;

	/** In the match: the round starts once every player has loaded in, or after this many seconds. */
	UPROPERTY(config, EditAnywhere, Category = "Match", meta = (ClampMin = "1"))
	float MatchStartTimeoutSeconds = 60.f;
};
