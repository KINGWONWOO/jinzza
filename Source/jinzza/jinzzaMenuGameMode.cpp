// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMenuGameMode.h"
#include "jinzzaMenuPlayerController.h"

AjinzzaMenuGameMode::AjinzzaMenuGameMode()
{
	PlayerControllerClass = AjinzzaMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}
