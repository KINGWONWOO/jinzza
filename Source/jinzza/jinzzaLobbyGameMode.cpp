// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyGameMode.h"
#include "jinzzaLobbyPlayerController.h"
#include "GameFramework/DefaultPawn.h"

AjinzzaLobbyGameMode::AjinzzaLobbyGameMode()
{
	PlayerControllerClass = AjinzzaLobbyPlayerController::StaticClass();
	DefaultPawnClass = ADefaultPawn::StaticClass();
	bUseSeamlessTravel = true;
}
