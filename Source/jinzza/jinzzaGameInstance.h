// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
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
 * GameInstance that owns the Steam online session lifecycle: hosting, quick-joining,
 * and destroying sessions. Bound to by the main menu and lobby UI for status feedback.
 */
UCLASS()
class JINZZA_API UjinzzaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Creates a Steam session (destroying any existing one first) and, on success, ServerTravels to Lvl_Lobby. */
	void HostSession();

	/** Searches for an existing Steam session and joins the first result found. */
	void QuickJoinSession();

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
	void FindSessionsInternal();

	void OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName InSessionName, bool bWasSuccessful);

	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;

	/** Set when DestroySession was called in order to start a fresh host/join, so completion chains into it. */
	bool bDestroyingToHost = false;
	bool bDestroyingToJoin = false;
};
