// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "jinzzaLobbyGameMode.generated.h"

/** GameMode for Lvl_Lobby: a waiting room with a simple pawn and the lobby UI. */
UCLASS()
class JINZZA_API AjinzzaLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AjinzzaLobbyGameMode();

	static const TCHAR* MatchMapPath;

	/**
	 * Host pressed Start Match: every player preloads the match level behind their loading screen
	 * (AjinzzaLobbyPlayerController::Client_PrepareForMatch) and the host travels only once all of them
	 * have reported back - or after UjinzzaLoadingSettings::MatchPreloadTimeoutSeconds.
	 */
	void BeginMatchPreparation();

	void NotifyMatchPreloaded(APlayerController* Player);

protected:
	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

private:
	void CheckMatchPreparation();
	void TravelToMatch();

	bool bPreparingMatch = false;
	bool bTravellingToMatch = false;
	TSet<TWeakObjectPtr<APlayerController>> PreloadedPlayers;
	FTimerHandle MatchPreloadTimeoutHandle;
};
