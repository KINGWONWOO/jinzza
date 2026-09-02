// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyGameMode.h"
#include "jinzzaLobbyPlayerController.h"
#include "jinzzaLobbyGameState.h"
#include "jinzzaGameInstance.h"
#include "jinzzaPartyPlayerState.h"
#include "GameFramework/DefaultPawn.h"

AjinzzaLobbyGameMode::AjinzzaLobbyGameMode()
{
	PlayerControllerClass = AjinzzaLobbyPlayerController::StaticClass();
	DefaultPawnClass = ADefaultPawn::StaticClass();
	GameStateClass = AjinzzaLobbyGameState::StaticClass();
	// Same PlayerState class Lvl_Game uses (AjinzzaGameGameMode), set here too so seamless travel
	// to Lvl_Game never has to worry about whether the carried-over PlayerState is still the base
	// APlayerState - AjinzzaGameGameMode::AssignRoles() casts every PlayerState to this type.
	PlayerStateClass = AjinzzaPartyPlayerState::StaticClass();
	bUseSeamlessTravel = true;
}

void AjinzzaLobbyGameMode::InitGameState()
{
	Super::InitGameState();

	if (AjinzzaLobbyGameState* LobbyGameState = GetGameState<AjinzzaLobbyGameState>())
	{
		if (const UjinzzaGameInstance* GI = GetGameInstance<UjinzzaGameInstance>())
		{
			LobbyGameState->MatchSettings = GI->GetPendingMatchSettings();
		}
	}
}
