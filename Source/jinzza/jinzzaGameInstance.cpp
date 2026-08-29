// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "jinzza.h"

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
		FindSessionsCompleteHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
			FOnFindSessionsCompleteDelegate::CreateUObject(this, &UjinzzaGameInstance::OnFindSessionsComplete));
		JoinSessionCompleteHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(this, &UjinzzaGameInstance::OnJoinSessionComplete));
		DestroySessionCompleteHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UjinzzaGameInstance::OnDestroySessionComplete));
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
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}

	Super::Shutdown();
}

void UjinzzaGameInstance::HostSession()
{
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
	Settings.NumPublicConnections = 12;
	Settings.NumPrivateConnections = 0;
	Settings.Set(FName(TEXT("MATCHTYPE")), FString(TEXT("JINZZA")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

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

	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(TEXT("/Game/JINZZA/Level/Lvl_Lobby?listen"));
	}
}

void UjinzzaGameInstance::QuickJoinSession()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("Online subsystem unavailable"));
		return;
	}

	if (Sessions->GetNamedSession(SessionName) != nullptr)
	{
		bDestroyingToJoin = true;
		Sessions->DestroySession(SessionName);
		return;
	}

	FindSessionsInternal();
}

void UjinzzaGameInstance::FindSessionsInternal()
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

	OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Searching, TEXT("Searching for sessions..."));

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = 20;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(FName(TEXT("MATCHTYPE")), FString(TEXT("JINZZA")), EOnlineComparisonOp::Equals);

	if (!Sessions->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef()))
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("Failed to start session search"));
	}
}

void UjinzzaGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful || !SessionSearch.IsValid() || SessionSearch->SearchResults.Num() == 0)
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("No sessions found"));
		return;
	}

	IOnlineSessionPtr Sessions = GetSessionInterface();
	const ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
	if (!Sessions.IsValid() || !LocalPlayer)
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("No local player"));
		return;
	}

	OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Joining, TEXT("Joining session..."));
	Sessions->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), SessionName, SessionSearch->SearchResults[0]);
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

	IOnlineSessionPtr Sessions = GetSessionInterface();
	APlayerController* PC = GetFirstLocalPlayerController();
	FString ConnectString;
	if (Sessions.IsValid() && PC && Sessions->GetResolvedConnectString(SessionName, ConnectString))
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Connected, TEXT("Connecting..."));
		PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
	}
	else
	{
		OnSessionStatusChanged.Broadcast(EJinzzaSessionStatus::Failed, TEXT("Failed to resolve connection"));
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

	if (bDestroyingToJoin)
	{
		bDestroyingToJoin = false;
		FindSessionsInternal();
		return;
	}
}
