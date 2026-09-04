// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyGameMode.h"
#include "jinzzaLobbyPlayerController.h"
#include "jinzzaLobbyGameState.h"
#include "jinzzaGameInstance.h"
#include "jinzzaPartyPlayerState.h"
#include "GameFramework/DefaultPawn.h"
#include "UObject/ConstructorHelpers.h"

AjinzzaLobbyGameMode::AjinzzaLobbyGameMode()
{
	PlayerControllerClass = AjinzzaLobbyPlayerController::StaticClass();
	GameStateClass = AjinzzaLobbyGameState::StaticClass();

	// Lvl_Lobby used to spawn a bare flying ADefaultPawn - invisible, so players in the pre-match
	// lobby couldn't see each other's characters standing around (same bug fixed for Lvl_Game in
	// AjinzzaGameGameMode; see its constructor comment). Reuses the same first-person character BP
	// so the pawn players get in the lobby is the same one they'll carry into the match.
	static ConstructorHelpers::FClassFinder<APawn> CharacterBPClass(TEXT("/Game/JINZZA/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	if (CharacterBPClass.Succeeded())
	{
		DefaultPawnClass = CharacterBPClass.Class;
	}
	else
	{
		DefaultPawnClass = ADefaultPawn::StaticClass();
	}
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
