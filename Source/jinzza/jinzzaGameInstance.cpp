// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "jinzza.h"

namespace
{
	const FName SettingRoomName(TEXT("ROOMNAME"));
	const FName SettingMaxPlayers(TEXT("MAXPLAYERS"));
	const FName SettingJudgeCount(TEXT("JUDGECOUNT"));
	const FName SettingVoteCount(TEXT("VOTECOUNT"));
	const FName SettingPhaseSpeed(TEXT("PHASESPEED"));
	const FName SettingRoleAssign(TEXT("ROLEASSIGN"));
	const FName SettingMatchType(TEXT("MATCHTYPE"));
}

const FName UjinzzaGameInstance::SessionName(TEXT("JinzzaSession"));

IOnlineSessionPtr UjinzzaGameInstance::GetSessionInterface() const
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		return Subsystem->GetSessionInterface();
	}
	return nullptr;
}

void UjinzzaGameInstance::Init()
{
	Super::Init();

	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		CreateSessionCompleteHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
			FOnCreateSessionCompleteDelegate::CreateUObject(this, &UjinzzaGameInstance::OnCreateSessionComplete));
		JoinSessionCompleteHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(this, &UjinzzaGameInstance::OnJoinSessionComplete));
		DestroySessionCompleteHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UjinzzaGameInstance::OnDestroySessionComplete));
		UpdateSessionCompleteHandle = Sessions->AddOnUpdateSessionCompleteDelegate_Handle(
			FOnUpdateSessionCompleteDelegate::CreateUObject(this, &UjinzzaGameInstance::OnUpdateSessionComplete));
		SessionUserInviteAcceptedHandle = Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(
			FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UjinzzaGameInstance::OnSessionUserInviteAccepted));
	}
	else
	{
		UE_LOG(Logjinzza, Warning, TEXT("No online subsystem session interface at startup - Steam sessions unavailable."));
	}
}

void UjinzzaGameInstance::Shutdown()
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		Sessions->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteHandle);
		Sessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(SessionUserInviteAcceptedHandle);
	}

	Super::Shutdown();
}

void UjinzzaGameInstance::HostSession(const FJinzzaMatchSettings& Settings)
{
	PendingMatchSettings = Settings;

	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("Online subsystem unavailable"));
		return;
	}

	if (Sessions->GetNamedSession(SessionName) != nullptr)
	{
		bDestroyingToHost = true;
		Sessions->DestroySession(SessionName);
		return;
	}

	CreateSessionInternal();
}

void UjinzzaGameInstance::CreateSessionInternal()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
	if (!LocalPlayer)
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("No local player"));
		return;
	}

	OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Creating, TEXT("Creating session..."));

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = false;
	Settings.bUsesPresence = true;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.NumPublicConnections = FMath::Clamp(PendingMatchSettings.MaxPlayers, 1, 12);
	Settings.NumPrivateConnections = 0;
	Settings.Set(SettingMatchType, FString(TEXT("JINZZA")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(SettingRoomName, PendingMatchSettings.RoomName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(SettingMaxPlayers, PendingMatchSettings.MaxPlayers, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(SettingJudgeCount, PendingMatchSettings.JudgeCount, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(SettingVoteCount, PendingMatchSettings.VoteCount, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(SettingPhaseSpeed, PendingMatchSettings.PhaseSpeed, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(SettingRoleAssign, PendingMatchSettings.RoleAssignMethod, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	if (!Sessions->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), SessionName, Settings))
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("Failed to start session creation"));
	}
}

void UjinzzaGameInstance::OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful)
{
	if (InSessionName != SessionName)
	{
		return;
	}

	if (!bWasSuccessful)
	{
		UE_LOG(Logjinzza, Error, TEXT("CreateSession failed."));
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("Failed to create session"));
		return;
	}

	OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Connected, TEXT("Session created - travelling to Lobby"));
	SetRichPresenceStatus(TEXT("In Lobby"));

	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(TEXT("/Game/JINZZA/Level/Lvl_Lobby?listen"));
	}
}

void UjinzzaGameInstance::UpdateLiveSessionSettings(const FJinzzaMatchSettings& Settings)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		return;
	}

	FNamedOnlineSession* CurrentSession = Sessions->GetNamedSession(SessionName);
	if (!CurrentSession)
	{
		// No live session yet (e.g. settings edited before the session finished creating) - nothing to push.
		return;
	}

	FOnlineSessionSettings UpdatedSettings = CurrentSession->SessionSettings;
	UpdatedSettings.NumPublicConnections = FMath::Clamp(Settings.MaxPlayers, 1, 12);
	UpdatedSettings.Set(SettingRoomName, Settings.RoomName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(SettingMaxPlayers, Settings.MaxPlayers, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(SettingJudgeCount, Settings.JudgeCount, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(SettingVoteCount, Settings.VoteCount, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(SettingPhaseSpeed, Settings.PhaseSpeed, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(SettingRoleAssign, Settings.RoleAssignMethod, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	Sessions->UpdateSession(SessionName, UpdatedSettings, true);
}

void UjinzzaGameInstance::OnUpdateSessionComplete(FName InSessionName, bool bWasSuccessful)
{
	if (InSessionName != SessionName)
	{
		return;
	}

	if (!bWasSuccessful)
	{
		UE_LOG(Logjinzza, Warning, TEXT("UpdateSession failed - advertised session data may be stale until the next successful update."));
	}
}

void UjinzzaGameInstance::SetRichPresenceStatus(const FString& StatusText)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		return;
	}

	IOnlinePresencePtr Presence = Subsystem->GetPresenceInterface();
	const ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
	if (!Presence.IsValid() || !LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		return;
	}

	FOnlineUserPresenceStatus NewStatus;
	NewStatus.StatusStr = StatusText;
	NewStatus.State = EOnlinePresenceState::Online;
	Presence->SetPresence(*LocalPlayer->GetPreferredUniqueNetId(), NewStatus);
}

void UjinzzaGameInstance::InviteFriends()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("Online subsystem unavailable"));
		return;
	}

	IOnlineExternalUIPtr ExternalUI = Subsystem->GetExternalUIInterface();
	if (!ExternalUI.IsValid() || !ExternalUI->ShowInviteUI(0, SessionName))
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("Could not open the Steam invite dialog"));
	}
}

void UjinzzaGameInstance::OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!bWasSuccessful || !UserId.IsValid() || !Sessions.IsValid() || !InviteResult.IsValid())
	{
		return;
	}

	if (Sessions->GetNamedSession(SessionName) != nullptr)
	{
		bJoiningFromInvite = true;
		PendingInviteResult = InviteResult;
		Sessions->DestroySession(SessionName);
		return;
	}

	OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Joining, TEXT("Joining invite..."));
	Sessions->JoinSession(*UserId, SessionName, InviteResult);
}

void UjinzzaGameInstance::OnJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (InSessionName != SessionName)
	{
		return;
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(Logjinzza, Error, TEXT("JoinSession failed with result %d"), static_cast<int32>(Result));
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("Failed to join session"));
		return;
	}

	TravelToConnectedSession();
}

void UjinzzaGameInstance::TravelToConnectedSession()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	APlayerController* PC = GetFirstLocalPlayerController();
	FString ConnectString;
	if (Sessions.IsValid() && PC && Sessions->GetResolvedConnectString(SessionName, ConnectString))
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Connected, TEXT("Connecting..."));
		SetRichPresenceStatus(TEXT("In Lobby"));
		PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
	}
	else
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("Failed to resolve connection"));
	}
}

void UjinzzaGameInstance::EndGameReturnToLobby()
{
	if (UWorld* World = GetWorld())
	{
		if (World->GetAuthGameMode() != nullptr)
		{
			World->ServerTravel(TEXT("/Game/JINZZA/Level/Lvl_Lobby"));
		}
	}
}

void UjinzzaGameInstance::DestroySession()
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		if (Sessions->GetNamedSession(SessionName) != nullptr)
		{
			Sessions->DestroySession(SessionName);
		}
	}
}

void UjinzzaGameInstance::OnDestroySessionComplete(FName InSessionName, bool bWasSuccessful)
{
	if (InSessionName != SessionName)
	{
		return;
	}

	if (bDestroyingToHost)
	{
		bDestroyingToHost = false;
		CreateSessionInternal();
		return;
	}

	if (bJoiningFromInvite)
	{
		bJoiningFromInvite = false;
		if (IOnlineSessionPtr Sessions = GetSessionInterface())
		{
			if (const ULocalPlayer* LocalPlayer = GetFirstGamePlayer())
			{
				OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Joining, TEXT("Joining invite..."));
				Sessions->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), SessionName, PendingInviteResult);
			}
		}
		return;
	}
}
