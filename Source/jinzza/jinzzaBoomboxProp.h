// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaInteractableProp.h"
#include "jinzzaBoomboxProp.generated.h"

class UAudioComponent;
class USoundBase;
class UMediaPlayer;
class UMediaSoundComponent;

/** One built-in song the boombox can play (see AjinzzaBoomboxProp::Playlist). */
USTRUCT(BlueprintType)
struct FJinzzaBoomboxTrack
{
	GENERATED_BODY()

	/** Name shown in the music player panel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boombox")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boombox")
	TObjectPtr<USoundBase> Sound = nullptr;
};

/**
 * What the boombox is currently set to play - the one replicated piece of state, so every client (and late
 * joiners) end up doing exactly the same thing. A source is EITHER a built-in track (TrackIndex >= 0, Url
 * empty) OR a streamed link (Url non-empty, TrackIndex ignored). Both empty = nothing chosen yet.
 */
USTRUCT(BlueprintType)
struct FJinzzaBoomboxMusicState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Boombox")
	int32 TrackIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Boombox")
	FString Url;

	UPROPERTY(BlueprintReadOnly, Category = "Boombox")
	bool bPlaying = false;

	bool HasSource() const { return !Url.IsEmpty() || TrackIndex >= 0; }
	bool IsSameSource(int32 OtherTrackIndex, const FString& OtherUrl) const
	{
		return Url.IsEmpty() ? (OtherUrl.IsEmpty() && TrackIndex == OtherTrackIndex) : Url.Equals(OtherUrl, ESearchCase::CaseSensitive);
	}
};

/**
 * Boombox: a handheld music player. Left click (the normal "use") still just toggles the current song
 * on/off; pressing the interact key AGAIN while holding it opens a music player panel
 * (UjinzzaBoomboxWidget) where the holder can pick a built-in song or paste a direct link to an audio file,
 * and left-click a song to play / pause it.
 *
 * Every machine plays the replicated MusicState locally (audio is positional at the boombox) - the server
 * only decides what it is. Only whoever is holding the boombox can change it, via
 * AjinzzaCharacter::RequestBoomboxMusic (props aren't owned by any client, so they can't take RPCs
 * themselves).
 *
 * Links: streamed through Unreal's Media Framework (needs the WmfMedia plugin on Windows), so they have to
 * be a DIRECT http(s) link to an audio file the platform decoder understands (.mp3, .aac, .m4a, .wav,
 * .wma, ...). Page links (YouTube, Spotify, SoundCloud, ...) cannot be played - there is no way to get an
 * audio stream out of them without an external extractor. Every nearby player's client fetches the link
 * itself, so the link's host can see those players' IP addresses.
 */
UCLASS()
class JINZZA_API AjinzzaBoomboxProp : public AjinzzaInteractableProp
{
	GENERATED_BODY()

public:
	AjinzzaBoomboxProp();

	UFUNCTION(BlueprintPure, Category = "Boombox")
	bool IsPlaying() const { return MusicState.bPlaying; }

	const FJinzzaBoomboxMusicState& GetMusicState() const { return MusicState; }
	const TArray<FJinzzaBoomboxTrack>& GetPlaylist() const { return Playlist; }

	/** Fired on every machine whenever MusicState changes (used by the music player panel to refresh itself). */
	FSimpleMulticastDelegate OnMusicStateChanged;

	/** True for a plain http:// or https:// link of sane length with no whitespace. */
	static bool IsAcceptableMusicUrl(const FString& Url);

	/**
	 * Server-only. Selects Source (TrackIndex, or Url when non-empty) and sets whether it is playing. Choosing
	 * a different source starts it from the beginning; choosing the source that's already current just
	 * plays / pauses it where it is. Invalid input (bad index, unacceptable link) is ignored.
	 */
	void ServerApplyMusic(int32 TrackIndex, const FString& Url, bool bPlaying);

	virtual UUserWidget* CreateHeldInteractionWidget(APlayerController* Interactor) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnPropActivated_Implementation() override;

	UFUNCTION()
	void OnRep_MusicState();

	UFUNCTION()
	void HandleMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void HandleMediaOpenFailed(FString FailedUrl);

private:
	/** Makes this machine's audio match MusicState. */
	void ApplyMusicLocally();

	void EnsureMediaPlayer();
	void StopAllPlayback();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UAudioComponent> AudioComponent;

	/** Built-in songs offered in the music player panel. Defaults to the project's existing music; override in the Blueprint to change. */
	UPROPERTY(EditAnywhere, Category = "Boombox")
	TArray<FJinzzaBoomboxTrack> Playlist;

	/** Legacy single track - only used (as the first playlist entry) if Playlist is left empty. */
	UPROPERTY(EditAnywhere, Category = "Boombox")
	TObjectPtr<USoundBase> Track;

	UPROPERTY(ReplicatedUsing = OnRep_MusicState)
	FJinzzaBoomboxMusicState MusicState;

	// Created lazily the first time a link is played (most boomboxes never need them).
	UPROPERTY(Transient)
	TObjectPtr<UMediaPlayer> MediaPlayer;

	UPROPERTY(Transient)
	TObjectPtr<UMediaSoundComponent> MediaSound;

	/** What THIS machine last started playing - local only, used to tell "new source" from "same source, just play/pause". */
	int32 AppliedTrackIndex = -1;
	FString AppliedUrl;
	bool bHasAppliedSource = false;
};
