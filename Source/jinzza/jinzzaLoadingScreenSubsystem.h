// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "Engine/EngineBaseTypes.h"
#include "Containers/Ticker.h"
#include "Styling/SlateBrush.h"
#include "jinzzaLoadingScreenSubsystem.generated.h"

class SjinzzaLoadingScreen;
class UGameViewportClient;
class UTexture2D;
struct FWorldContext;

/**
 * Owns the loading screen (SjinzzaLoadingScreen) for every level change, per local game instance.
 *
 * Stages:
 *   Preparing  - lobby -> match only (BeginMatchPreload): every player preloads the match level's
 *                assets in the lobby; the host travels once all of them reported done
 *                (AjinzzaLobbyGameMode). Shows "Waiting for other players (x/y)".
 *   Travelling - the map itself is loading. A blocking load (main menu <-> lobby, joining a host) is
 *                covered by the MoviePlayer copy of the screen, since the game thread can't draw then;
 *                a seamless travel (lobby -> match) by the viewport copy.
 *   Arriving   - map loaded: preload that map's asset list (UjinzzaLoadingSettings) + system data
 *                (user settings, rich presence, friends list), wait for our own controller/pawn, then
 *                hide. In the match it also tells the server we're in
 *                (AjinzzaGamePlayerController::ReportLoadComplete) and stays up until the server says
 *                every player is in (AjinzzaGameGameState::AreAllPlayersLoaded) - so everyone sees the
 *                match at the same moment and the round starts then. Elsewhere (main menu -> lobby,
 *                joining) nobody waits for anyone: the screen goes as soon as this player is ready.
 */
UCLASS()
class JINZZA_API UjinzzaLoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Lobby -> match: shows the screen and preloads MapPath's assets. OnComplete runs once, when done. */
	void BeginMatchPreload(const FString& MapPath, TFunction<void()> OnComplete);

	/** Shows the screen ahead of a travel the engine hasn't started yet (e.g. while connecting to a host). */
	void ShowForTravel(const FText& InStatus);

	bool IsShowing() const { return Stage != EStage::Hidden; }

private:
	enum class EStage : uint8
	{
		Hidden,
		Preparing,
		Travelling,
		Arriving,
	};

	void HandlePreLoadMap(const FWorldContext& WorldContext, const FString& MapName);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void HandleSeamlessTravelStart(UWorld* CurrentWorld, const FString& MapName);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	bool Tick(float DeltaTime);
	void TickPreparing();
	void TickArriving(double Now);

	void Show();
	void Hide();
	void AttachToViewport();
	TSharedRef<SjinzzaLoadingScreen> MakeScreen(bool bLive);
	void SetupMoviePlayer();

	void StartLevelAssetPreload(const FString& MapName);
	void PreloadSystemData(const FString& MapName);
	bool IsLocalPlayerReady(UWorld* World, double Now) const;

	FText GetStatusText() const { return StatusText; }
	TOptional<float> GetProgress() const { return Progress; }

	static FString ToShortMapName(const FString& MapName);
	static bool IsHandleDone(const TSharedPtr<FStreamableHandle>& Handle);

	EStage Stage = EStage::Hidden;

	/** Destination of the current travel (short name), so a transition map's load is ignored. Empty = unknown. */
	FString PendingMapName;

	double ShownTime = 0.0;
	double StageStartTime = 0.0;
	bool bReportedLoaded = false;

	FText StatusText;
	TOptional<float> Progress;
	FText CurrentTip;

	TFunction<void()> PendingMatchPreloadDone;

	FStreamableManager StreamableManager;
	/** The current level's preload list - kept so those assets stay resident while we're in it. */
	TSharedPtr<FStreamableHandle> LevelAssetsHandle;
	/** Arrival preload in flight; becomes LevelAssetsHandle once done. */
	TSharedPtr<FStreamableHandle> PendingAssetsHandle;
	/** Lobby -> match preload of the match map's dependencies, kept until the match level is loaded. */
	TSharedPtr<FStreamableHandle> MatchPreloadHandle;

	TSharedPtr<SjinzzaLoadingScreen> ViewportScreen;
	TWeakObjectPtr<UGameViewportClient> ScreenViewport;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> LogoTexture;

	/** Keeps the loading screen's font asset alive for the MoviePlayer copy (drawn off the game thread). */
	UPROPERTY(Transient)
	TObjectPtr<const UObject> FontObject;

	FSlateBrush LogoBrush;

	FTSTicker::FDelegateHandle TickHandle;
	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle SeamlessTravelStartHandle;
	FDelegateHandle TravelFailureHandle;
};
