// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLoadingScreenSubsystem.h"
#include "SjinzzaLoadingScreen.h"
#include "jinzzaLoadingSettings.h"
#include "jinzzaGameInstance.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaGameGameState.h"
#include "jinzzaLobbyGameState.h"
#include "jinzzaGamePlayerController.h"
#include "jinzzaPlayerController.h"
#include "jinzzaUIStyle.h"
#include "jinzza.h"
#include "MoviePlayer.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"

namespace
{
	constexpr int32 LoadingScreenZOrder = 10000;
	constexpr double PawnWaitSeconds = 10.0;

	const TCHAR* LogoPath = TEXT("/Game/JINZZA/UI/Textures/T_Logo.T_Logo");

	/** Every on-disk asset the map package directly hard-references, under /Game. Loading those pulls in
	 * their own dependencies too, so this warms almost everything the map needs without loading the map
	 * package itself (seamless travel must load that one itself - it sets the world's type while loading). */
	TArray<FSoftObjectPath> GatherMapDependencies(const FString& MapPackageName)
	{
		TArray<FSoftObjectPath> Result;
		IAssetRegistry* Registry = IAssetRegistry::Get();
		if (!Registry)
		{
			return Result;
		}

		TArray<FName> Dependencies;
		Registry->GetDependencies(FName(*MapPackageName), Dependencies, UE::AssetRegistry::EDependencyCategory::Package,
			UE::AssetRegistry::EDependencyQuery::Hard);

		for (const FName& Dependency : Dependencies)
		{
			const FString DependencyName = Dependency.ToString();
			if (!DependencyName.StartsWith(TEXT("/Game/")) || DependencyName == MapPackageName)
			{
				continue;
			}

			TArray<FAssetData> Assets;
			Registry->GetAssetsByPackageName(Dependency, Assets, true);
			for (const FAssetData& Asset : Assets)
			{
				// Other maps (e.g. sublevels) load with the world, not as plain assets.
				if (!Asset.IsInstanceOf<UWorld>())
				{
					Result.Add(Asset.GetSoftObjectPath());
				}
			}
		}
		return Result;
	}
}

bool UjinzzaLoadingScreenSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer() && Super::ShouldCreateSubsystem(Outer);
}

void UjinzzaLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LogoTexture = LoadObject<UTexture2D>(nullptr, LogoPath);
	if (LogoTexture)
	{
		LogoBrush.SetResourceObject(LogoTexture);
		LogoBrush.ImageSize = FVector2D(LogoTexture->GetSizeX(), LogoTexture->GetSizeY());
		LogoBrush.DrawAs = ESlateBrushDrawType::Image;
	}
	FontObject = JinzzaUI::BodyFont(22).FontObject;

	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &UjinzzaLoadingScreenSubsystem::HandlePreLoadMap);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UjinzzaLoadingScreenSubsystem::HandlePostLoadMap);
	SeamlessTravelStartHandle = FWorldDelegates::OnSeamlessTravelStart.AddUObject(this, &UjinzzaLoadingScreenSubsystem::HandleSeamlessTravelStart);
	if (GEngine)
	{
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UjinzzaLoadingScreenSubsystem::HandleTravelFailure);
	}

	// Core ticker, not a world tick: it keeps running during seamless travel and between worlds.
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UjinzzaLoadingScreenSubsystem::Tick));
}

void UjinzzaLoadingScreenSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadMapHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	FWorldDelegates::OnSeamlessTravelStart.Remove(SeamlessTravelStartHandle);
	if (GEngine)
	{
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}

	Hide();
	LevelAssetsHandle.Reset();
	PendingAssetsHandle.Reset();
	MatchPreloadHandle.Reset();

	Super::Deinitialize();
}

FString UjinzzaLoadingScreenSubsystem::ToShortMapName(const FString& MapName)
{
	FString Name = MapName;
	int32 OptionsStart = INDEX_NONE;
	if (Name.FindChar(TEXT('?'), OptionsStart))
	{
		Name.LeftInline(OptionsStart);
	}
	return UWorld::RemovePIEPrefix(FPackageName::GetShortName(Name));
}

bool UjinzzaLoadingScreenSubsystem::IsHandleDone(const TSharedPtr<FStreamableHandle>& Handle)
{
	return !Handle.IsValid() || Handle->HasLoadCompleted() || Handle->WasCanceled();
}

// --- Entry points ---------------------------------------------------------------------------------

void UjinzzaLoadingScreenSubsystem::BeginMatchPreload(const FString& MapPath, TFunction<void()> OnComplete)
{
	Stage = EStage::Preparing;
	StageStartTime = FPlatformTime::Seconds();
	PendingMapName = ToShortMapName(MapPath);
	PendingMatchPreloadDone = MoveTemp(OnComplete);
	StatusText = FText::FromString(TEXT("Preparing the match..."));
	Progress.Reset();

	TArray<FSoftObjectPath> Assets = GatherMapDependencies(MapPath);
	Assets.Append(UjinzzaLoadingSettings::Get()->GetPreloadAssetsForMap(PendingMapName));
	MatchPreloadHandle = Assets.Num() > 0 ? StreamableManager.RequestAsyncLoad(Assets, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority) : nullptr;
	UE_LOG(Logjinzza, Log, TEXT("Match preload: %d assets for %s."), Assets.Num(), *PendingMapName);

	Show();
}

void UjinzzaLoadingScreenSubsystem::ShowForTravel(const FText& InStatus)
{
	Stage = EStage::Travelling;
	StageStartTime = FPlatformTime::Seconds();
	PendingMapName.Empty();
	StatusText = InStatus;
	Progress.Reset();
	Show();
}

// --- Engine travel events -------------------------------------------------------------------------

void UjinzzaLoadingScreenSubsystem::HandlePreLoadMap(const FWorldContext& WorldContext, const FString& MapName)
{
	if (WorldContext.OwningGameInstance != GetGameInstance())
	{
		return;
	}

	Stage = EStage::Travelling;
	StageStartTime = FPlatformTime::Seconds();
	PendingMapName = ToShortMapName(MapName);
	StatusText = FText::FromString(TEXT("Loading..."));
	Progress.Reset();
	Show();

	// The game thread is about to block inside LoadMap, so the viewport can't draw - the MoviePlayer
	// draws its own copy of the screen on another thread until the map is in. Not available in the editor.
	SetupMoviePlayer();
}

void UjinzzaLoadingScreenSubsystem::HandleSeamlessTravelStart(UWorld* CurrentWorld, const FString& MapName)
{
	if (!CurrentWorld || CurrentWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	// The world keeps ticking during seamless travel, so the viewport copy keeps drawing on its own.
	Stage = EStage::Travelling;
	StageStartTime = FPlatformTime::Seconds();
	PendingMapName = ToShortMapName(MapName);
	StatusText = FText::FromString(PendingMapName == TEXT("Lvl_Game") ? TEXT("Loading the match...") : TEXT("Loading..."));
	Progress.Reset();
	Show();
}

void UjinzzaLoadingScreenSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (Stage == EStage::Hidden)
	{
		return;
	}
	if (!LoadedWorld)
	{
		// Load failed - the engine falls back to another map, which shows the screen again itself.
		Hide();
		return;
	}
	if (LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	const FString MapName = ToShortMapName(LoadedWorld->GetMapName());
	if (!PendingMapName.IsEmpty() && MapName != PendingMapName)
	{
		return;
	}

	Stage = EStage::Arriving;
	StageStartTime = FPlatformTime::Seconds();
	PendingMapName.Empty();
	bReportedLoaded = false;
	StatusText = FText::FromString(TEXT("Loading..."));
	Progress.Reset();

	// The level itself references what it uses now.
	MatchPreloadHandle.Reset();

	// LoadMap can leave the viewport rebuilt - make sure the screen is still on it.
	AttachToViewport();

	StartLevelAssetPreload(MapName);
	PreloadSystemData(MapName);
}

void UjinzzaLoadingScreenSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (World && World->GetGameInstance() != GetGameInstance())
	{
		return;
	}
	// The engine's fallback travel (if any) brings the screen back via PreLoadMap.
	Hide();
}

// --- Preloading -----------------------------------------------------------------------------------

void UjinzzaLoadingScreenSubsystem::StartLevelAssetPreload(const FString& MapName)
{
	const TArray<FSoftObjectPath> Assets = UjinzzaLoadingSettings::Get()->GetPreloadAssetsForMap(MapName);
	// The previous level's handle is only released once this one is done (see TickArriving), so assets
	// shared by both levels are never unloaded in between.
	PendingAssetsHandle = Assets.Num() > 0 ? StreamableManager.RequestAsyncLoad(Assets, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority) : nullptr;
}

void UjinzzaLoadingScreenSubsystem::PreloadSystemData(const FString& MapName)
{
	// Settings (audio volumes, key rebinds, mic mode, customization) - loads the config if it isn't yet.
	UjinzzaGameUserSettings::Get();

	UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(GetGameInstance());
	if (!GI)
	{
		return;
	}

	if (MapName == TEXT("Lvl_Lobby"))
	{
		GI->SetRichPresenceStatus(TEXT("In Lobby"));
		// Cached on the game instance, so the friend-invite kiosk opens with the list already there.
		GI->RequestFriendsList();
	}
	else if (MapName == TEXT("Lvl_Game"))
	{
		GI->SetRichPresenceStatus(TEXT("In Match"));
	}
	else if (MapName == TEXT("Lvl_MainMenu"))
	{
		GI->SetRichPresenceStatus(TEXT("In Menu"));
	}
}

bool UjinzzaLoadingScreenSubsystem::IsLocalPlayerReady(UWorld* World, double Now) const
{
	if (!World || !World->GetGameState() || !World->HasBegunPlay())
	{
		return false;
	}

	APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController(World);
	if (!PC || !PC->HasActorBegunPlay())
	{
		return false;
	}

	// Play levels give every player a character - wait for it so nobody sees an empty camera. The main
	// menu's controller has none. Don't wait forever if spawning failed.
	if (PC->IsA<AjinzzaPlayerController>() && !PC->GetPawn() && Now - StageStartTime < PawnWaitSeconds)
	{
		return false;
	}
	return true;
}

// --- Tick -----------------------------------------------------------------------------------------

bool UjinzzaLoadingScreenSubsystem::Tick(float DeltaTime)
{
	if (Stage == EStage::Hidden)
	{
		return true;
	}

	const double Now = FPlatformTime::Seconds();
	// Per stage: lobby preparation + travel + waiting for everyone can legitimately add up past the limit.
	if (Now - StageStartTime > UjinzzaLoadingSettings::Get()->MaximumDisplaySeconds)
	{
		UE_LOG(Logjinzza, Warning, TEXT("Loading screen timed out - hiding it."));
		Hide();
		return true;
	}

	switch (Stage)
	{
	case EStage::Preparing:
		TickPreparing();
		break;
	case EStage::Travelling:
		break;
	case EStage::Arriving:
		TickArriving(Now);
		break;
	default:
		break;
	}
	return true;
}

void UjinzzaLoadingScreenSubsystem::TickPreparing()
{
	if (!IsHandleDone(MatchPreloadHandle))
	{
		StatusText = FText::FromString(TEXT("Preparing the match..."));
		Progress = MatchPreloadHandle->GetProgress();
		return;
	}

	if (PendingMatchPreloadDone)
	{
		TFunction<void()> Done = MoveTemp(PendingMatchPreloadDone);
		PendingMatchPreloadDone = nullptr;
		Done();
	}

	UWorld* World = GetGameInstance()->GetWorld();
	const AjinzzaLobbyGameState* LobbyState = World ? World->GetGameState<AjinzzaLobbyGameState>() : nullptr;
	const int32 Total = LobbyState ? LobbyState->MatchPrepTotalCount : 0;
	const int32 Ready = LobbyState ? FMath::Min(LobbyState->MatchPrepReadyCount, Total) : 0;
	StatusText = FText::FromString(FString::Printf(TEXT("Waiting for other players (%d/%d)"), Ready, Total));
	Progress = Total > 0 ? TOptional<float>(float(Ready) / float(Total)) : TOptional<float>();
}

void UjinzzaLoadingScreenSubsystem::TickArriving(double Now)
{
	if (!IsHandleDone(PendingAssetsHandle))
	{
		StatusText = FText::FromString(TEXT("Loading assets..."));
		Progress = PendingAssetsHandle->GetProgress();
		return;
	}
	if (PendingAssetsHandle.IsValid())
	{
		LevelAssetsHandle = PendingAssetsHandle;
		PendingAssetsHandle.Reset();
	}

	UWorld* World = GetGameInstance()->GetWorld();
	if (!IsLocalPlayerReady(World, Now))
	{
		StatusText = FText::FromString(TEXT("Getting ready..."));
		Progress.Reset();
		return;
	}

	if (!bReportedLoaded)
	{
		bReportedLoaded = true;
		if (AjinzzaGamePlayerController* GamePC = Cast<AjinzzaGamePlayerController>(GetGameInstance()->GetFirstLocalPlayerController(World)))
		{
			GamePC->ReportLoadComplete();
		}
	}

	// The match waits for everybody; every other level only for this player.
	if (const AjinzzaGameGameState* MatchState = World->GetGameState<AjinzzaGameGameState>())
	{
		if (!MatchState->AreAllPlayersLoaded())
		{
			const int32 Total = MatchState->GetExpectedPlayerCount();
			const int32 Loaded = FMath::Min(MatchState->GetLoadedPlayerCount(), Total);
			StatusText = FText::FromString(FString::Printf(TEXT("Waiting for other players (%d/%d)"), Loaded, Total));
			Progress = Total > 0 ? TOptional<float>(float(Loaded) / float(Total)) : TOptional<float>();
			return;
		}
	}

	StatusText = FText::FromString(TEXT("Ready!"));
	Progress = 1.f;
	if (Now - ShownTime >= UjinzzaLoadingSettings::Get()->MinimumDisplaySeconds)
	{
		Hide();
	}
}

// --- Display --------------------------------------------------------------------------------------

TSharedRef<SjinzzaLoadingScreen> UjinzzaLoadingScreenSubsystem::MakeScreen(bool bLive)
{
	TAttribute<FText> Status = bLive
		? TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateUObject(this, &UjinzzaLoadingScreenSubsystem::GetStatusText))
		: TAttribute<FText>(FText::FromString(TEXT("Loading...")));
	TAttribute<TOptional<float>> Percent = bLive
		? TAttribute<TOptional<float>>::Create(TAttribute<TOptional<float>>::FGetter::CreateUObject(this, &UjinzzaLoadingScreenSubsystem::GetProgress))
		: TAttribute<TOptional<float>>(TOptional<float>());

	return SNew(SjinzzaLoadingScreen)
		.LogoBrush(LogoTexture ? &LogoBrush : nullptr)
		.Tip(CurrentTip)
		.TitleFont(JinzzaUI::TitleFont(64))
		.BodyFont(JinzzaUI::BodyFont(22))
		.StatusText(Status)
		.Progress(Percent);
}

void UjinzzaLoadingScreenSubsystem::Show()
{
	if (!ViewportScreen.IsValid())
	{
		const TArray<FText>& Tips = UjinzzaLoadingSettings::Get()->Tips;
		CurrentTip = Tips.Num() > 0 ? Tips[FMath::RandRange(0, Tips.Num() - 1)] : FText::GetEmpty();
		ViewportScreen = MakeScreen(true);
		ShownTime = FPlatformTime::Seconds();
	}
	AttachToViewport();
}

void UjinzzaLoadingScreenSubsystem::AttachToViewport()
{
	UGameViewportClient* Viewport = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr;
	if (!Viewport || !ViewportScreen.IsValid())
	{
		return;
	}

	if (UGameViewportClient* Previous = ScreenViewport.Get())
	{
		Previous->RemoveViewportWidgetContent(ViewportScreen.ToSharedRef());
	}
	Viewport->AddViewportWidgetContent(ViewportScreen.ToSharedRef(), LoadingScreenZOrder);
	ScreenViewport = Viewport;
}

void UjinzzaLoadingScreenSubsystem::Hide()
{
	if (ViewportScreen.IsValid())
	{
		if (UGameViewportClient* Viewport = ScreenViewport.Get())
		{
			Viewport->RemoveViewportWidgetContent(ViewportScreen.ToSharedRef());
		}
	}
	ViewportScreen.Reset();
	ScreenViewport.Reset();

	Stage = EStage::Hidden;
	PendingMapName.Empty();
	PendingMatchPreloadDone = nullptr;
}

void UjinzzaLoadingScreenSubsystem::SetupMoviePlayer()
{
	if (GIsEditor || !IsMoviePlayerEnabled())
	{
		return;
	}

	IGameMoviePlayer* MoviePlayer = GetMoviePlayer();
	if (!MoviePlayer || MoviePlayer->IsMovieCurrentlyPlaying())
	{
		return;
	}

	// Runs before the MoviePlayer's own PreLoadMap handler (PreLoadMapWithContext is broadcast first),
	// which then starts playing this.
	FLoadingScreenAttributes Attributes;
	Attributes.bAutoCompleteWhenLoadingCompletes = true;
	Attributes.bMoviesAreSkippable = false;
	Attributes.bWaitForManualStop = false;
	Attributes.MinimumLoadingScreenDisplayTime = 0.f;
	Attributes.WidgetLoadingScreen = MakeScreen(false);
	MoviePlayer->SetupLoadingScreen(Attributes);
}
