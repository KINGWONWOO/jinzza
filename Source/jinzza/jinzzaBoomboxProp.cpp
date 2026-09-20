// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaBoomboxProp.h"
#include "jinzzaBoomboxWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerController.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AjinzzaBoomboxProp::AjinzzaBoomboxProp()
{
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(RootComponent);
	AudioComponent->bAutoActivate = false;

	// Default playlist = the music the project already has. Entries whose asset is missing are just skipped.
	struct FDefaultTrack
	{
		const TCHAR* Name;
		const TCHAR* Path;
	};
	static const FDefaultTrack DefaultTracks[] = {
		{ TEXT("Fruit Game BGM"), TEXT("/Game/JINZZA/Audio/Sounds/Boombox/FruitGameBgm.FruitGameBgm") },
		{ TEXT("Quiz Game BGM"), TEXT("/Game/JINZZA/Audio/Sounds/Game/QuizGameBgm.QuizGameBgm") },
		{ TEXT("Lobby BGM"), TEXT("/Game/JINZZA/Audio/Sounds/Lobby/LobbyBgm__cut_83sec__Cue.LobbyBgm__cut_83sec__Cue") },
		{ TEXT("Main Menu BGM"), TEXT("/Game/JINZZA/Audio/Sounds/MainMenu/MainMenuBgm_Cue.MainMenuBgm_Cue") },
	};
	for (const FDefaultTrack& DefaultTrack : DefaultTracks)
	{
		ConstructorHelpers::FObjectFinderOptional<USoundBase> Finder(DefaultTrack.Path);
		if (USoundBase* Sound = Finder.Get())
		{
			FJinzzaBoomboxTrack Entry;
			Entry.DisplayName = FText::FromString(DefaultTrack.Name);
			Entry.Sound = Sound;
			Playlist.Add(Entry);
		}
	}
}

void AjinzzaBoomboxProp::BeginPlay()
{
	Super::BeginPlay();

	// Blueprints that cleared the playlist but still have the legacy single Track set keep working.
	if (Playlist.Num() == 0 && Track)
	{
		FJinzzaBoomboxTrack Entry;
		Entry.DisplayName = FText::FromString(TEXT("Boombox"));
		Entry.Sound = Track;
		Playlist.Add(Entry);
	}
}

void AjinzzaBoomboxProp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllPlayback();

	Super::EndPlay(EndPlayReason);
}

void AjinzzaBoomboxProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AjinzzaBoomboxProp, MusicState);
}

bool AjinzzaBoomboxProp::IsAcceptableMusicUrl(const FString& Url)
{
	if (Url.IsEmpty() || Url.Len() > 512)
	{
		return false;
	}

	if (!Url.StartsWith(TEXT("http://"), ESearchCase::IgnoreCase) && !Url.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase))
	{
		return false;
	}

	for (const TCHAR Character : Url)
	{
		if (FChar::IsWhitespace(Character))
		{
			return false;
		}
	}
	return true;
}

void AjinzzaBoomboxProp::OnPropActivated_Implementation()
{
	// Plain "use" (left click): toggle whatever is currently chosen, starting the first built-in song if nothing is yet.
	FJinzzaBoomboxMusicState NewState = MusicState;
	if (!NewState.HasSource())
	{
		if (Playlist.Num() == 0)
		{
			return;
		}
		NewState.TrackIndex = 0;
		NewState.bPlaying = true;
	}
	else
	{
		NewState.bPlaying = !NewState.bPlaying;
	}

	MusicState = NewState;
	OnRep_MusicState();
}

void AjinzzaBoomboxProp::ServerApplyMusic(int32 TrackIndex, const FString& Url, bool bPlaying)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!Url.IsEmpty())
	{
		if (!IsAcceptableMusicUrl(Url))
		{
			return;
		}
	}
	else if (!Playlist.IsValidIndex(TrackIndex) || !Playlist[TrackIndex].Sound)
	{
		return;
	}

	FJinzzaBoomboxMusicState NewState;
	NewState.TrackIndex = Url.IsEmpty() ? TrackIndex : -1;
	NewState.Url = Url;
	NewState.bPlaying = bPlaying;

	if (MusicState.bPlaying == NewState.bPlaying && MusicState.IsSameSource(NewState.TrackIndex, NewState.Url))
	{
		return; // already exactly this
	}

	MusicState = NewState;
	OnRep_MusicState(); // the server doesn't get its own RepNotify
}

void AjinzzaBoomboxProp::OnRep_MusicState()
{
	ApplyMusicLocally();
	OnMusicStateChanged.Broadcast();
}

void AjinzzaBoomboxProp::ApplyMusicLocally()
{
	// A dedicated server has no audio device - only the replicated state matters there.
	if (IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	if (!MusicState.HasSource())
	{
		StopAllPlayback();
		bHasAppliedSource = false;
		return;
	}

	const bool bNewSource = !bHasAppliedSource || AppliedTrackIndex != MusicState.TrackIndex || AppliedUrl != MusicState.Url;
	if (bNewSource)
	{
		StopAllPlayback();
		AppliedTrackIndex = MusicState.TrackIndex;
		AppliedUrl = MusicState.Url;
		bHasAppliedSource = true;
	}

	if (MusicState.Url.IsEmpty())
	{
		// Built-in track
		USoundBase* Sound = Playlist.IsValidIndex(MusicState.TrackIndex) ? Playlist[MusicState.TrackIndex].Sound.Get() : nullptr;
		if (!Sound || !AudioComponent)
		{
			return;
		}

		if (bNewSource)
		{
			AudioComponent->SetSound(Sound);
		}

		if (MusicState.bPlaying)
		{
			if (AudioComponent->IsPlaying())
			{
				AudioComponent->SetPaused(false);
			}
			else
			{
				AudioComponent->Play();
			}
		}
		else if (AudioComponent->IsPlaying())
		{
			AudioComponent->SetPaused(true);
		}
	}
	else
	{
		// Streamed link
		EnsureMediaPlayer();
		if (!MediaPlayer)
		{
			return;
		}

		if (bNewSource)
		{
			MediaPlayer->OpenUrl(MusicState.Url); // starts playing from HandleMediaOpened if still switched on by then
		}
		else if (MusicState.bPlaying)
		{
			MediaPlayer->Play();
		}
		else
		{
			MediaPlayer->Pause();
		}
	}
}

void AjinzzaBoomboxProp::EnsureMediaPlayer()
{
	if (MediaPlayer)
	{
		return;
	}

	MediaPlayer = NewObject<UMediaPlayer>(this);
	if (!MediaPlayer)
	{
		return;
	}

	MediaPlayer->PlayOnOpen = false; // HandleMediaOpened decides, so a paused boombox doesn't start on its own
	MediaPlayer->SetLooping(true);
	MediaPlayer->OnMediaOpened.AddDynamic(this, &AjinzzaBoomboxProp::HandleMediaOpened);
	MediaPlayer->OnMediaOpenFailed.AddDynamic(this, &AjinzzaBoomboxProp::HandleMediaOpenFailed);

	MediaSound = NewObject<UMediaSoundComponent>(this, TEXT("MediaSound"));
	if (MediaSound)
	{
		MediaSound->SetMediaPlayer(MediaPlayer);
		MediaSound->bAllowSpatialization = true;
		MediaSound->bOverrideAttenuation = true;
		MediaSound->AttenuationOverrides.bAttenuate = true;
		MediaSound->AttenuationOverrides.bSpatialize = true;
		MediaSound->AttenuationOverrides.AttenuationShapeExtents = FVector(300.f, 0.f, 0.f);
		MediaSound->AttenuationOverrides.FalloffDistance = 1500.f;
		MediaSound->SetupAttachment(GetRootComponent());
		MediaSound->RegisterComponent();
	}
}

void AjinzzaBoomboxProp::StopAllPlayback()
{
	if (AudioComponent)
	{
		AudioComponent->Stop();
	}
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}
}

void AjinzzaBoomboxProp::HandleMediaOpened(FString OpenedUrl)
{
	if (MediaPlayer && MusicState.bPlaying && !MusicState.Url.IsEmpty())
	{
		MediaPlayer->Play();
	}
}

void AjinzzaBoomboxProp::HandleMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogTemp, Warning, TEXT("Boombox: could not open link '%s' (must be a direct link to an audio file the platform decoder supports)"), *FailedUrl);
}

UUserWidget* AjinzzaBoomboxProp::CreateHeldInteractionWidget(APlayerController* Interactor)
{
	if (!Interactor)
	{
		return nullptr;
	}

	UjinzzaBoomboxWidget* Widget = CreateWidget<UjinzzaBoomboxWidget>(Interactor, UjinzzaBoomboxWidget::StaticClass());
	if (Widget)
	{
		Widget->SetBoombox(this);
	}
	return Widget;
}
