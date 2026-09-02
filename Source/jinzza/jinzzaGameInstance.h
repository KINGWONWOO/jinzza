// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "jinzzaMatchSettings.h"
#include "jinzzaGameInstance.generated.h"

/** High-level status of the current Steam session operation, for UI feedback. */
UENUM(BlueprintType)
enum class EJinzzaSessionStatus : uint8
{
	Idle,
	Creating,
	Searching,
	Joining,
	Failed,
	Connected
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnJinzzaSessionStatusChanged, EJinzzaSessionStatus /*Status*/, const FString& /*Message*/);

/**
 * GameInstance that owns the Steam online session lifecycle: hosting with default match
 * settings, accepting Steam overlay invites, and destroying sessions. Bound to by the main
 * menu and lobby UI for status feedback.
 *
 * Joining is invite-only by design (see AjinzzaRoomSettingsKiosk for adjusting match settings
 * once in the lobby) - there is no room-list search/browse flow.
 */
UCLASS()
class JINZZA_API UjinzzaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Creates a Steam session using Settings (destroying any existing one first) and, on success, ServerTravels to Lvl_Lobby. */
	void HostSession(const FJinzzaMatchSettings& Settings);

	/** The match settings the current/most recent session was hosted with. */
	FJinzzaMatchSettings GetPendingMatchSettings() const { return PendingMatchSettings; }

	/** Updates the stored match settings without touching the live session (e.g. after the host edits them in the lobby). */
	void SetPendingMatchSettings(const FJinzzaMatchSettings& Settings) { PendingMatchSettings = Settings; }

	/**
	 * Host-only: pushes Settings to the already-created live Steam session (advertised player
	 * count and searchable match data) via IOnlineSession::UpdateSession, without destroying and
	 * recreating it. Called after the host edits settings at the in-lobby Room Settings kiosk, so
	 * e.g. a widened player-count cap is actually reflected in what Steam advertises to friends.
	 * No-op if there is no live session.
	 */
	void UpdateLiveSessionSettings(const FJinzzaMatchSettings& Settings);

	/** Opens the native Steam overlay invite dialog for the current session. */
	void InviteFriends();

	/**
	 * Publishes StatusText as the local player's Steam friends-list rich presence (shows up as
	 * e.g. "In Lobby" on a friend's friends list). No-ops cleanly if there's no presence-capable
	 * online subsystem or no signed-in local user. Round-phase text should feed through here once
	 * URoundPhaseSubsystem exists (design doc section 8-1's "Rich Presence" requirement).
	 */
	void SetRichPresenceStatus(const FString& StatusText);

	/** Host-only: travels everyone back to Lvl_Lobby, keeping the session alive. */
	void EndGameReturnToLobby();

	/** Destroys the current session, if any. */
	void DestroySession();

	/** Fired whenever the session flow changes state, for UI status text. */
	FOnJinzzaSessionStatusChanged OnSessionStatusChanged;

	static const FName SessionName;

protected:
	virtual void Init() override;
	virtual void Shutdown() override;

private:
	IOnlineSessionPtr GetSessionInterface() const;

	void CreateSessionInternal();

	void OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful);
	void OnJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName InSessionName, bool bWasSuccessful);
	void OnUpdateSessionComplete(FName InSessionName, bool bWasSuccessful);
	void OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);

	void TravelToConnectedSession();

	FJinzzaMatchSettings PendingMatchSettings;

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;
	FDelegateHandle UpdateSessionCompleteHandle;
	FDelegateHandle SessionUserInviteAcceptedHandle;

	/** Set when DestroySession was called in order to start a fresh host, so completion chains into it. */
	bool bDestroyingToHost = false;

	/** Set when the pending join came from an accepted Steam invite. */
	FOnlineSessionSearchResult PendingInviteResult;
	bool bJoiningFromInvite = false;
};
