// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "jinzzaMenuBackgroundCharacter.generated.h"

/**
 * A purely decorative, uncontrolled character shown behind the main menu UI - see
 * AjinzzaMenuPlayerController::SetupMenuBackgroundScene, which auto-spawns one (facing
 * AjinzzaMenuCameraRig) if none is hand-placed in Lvl_MainMenu. Never possessed by a controller;
 * mesh/animation are set on a Blueprint subclass (or a hand-placed instance) in the editor, same
 * "wire content later" pattern as this project's other decor actors.
 */
UCLASS()
class JINZZA_API AjinzzaMenuBackgroundCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AjinzzaMenuBackgroundCharacter();
};
