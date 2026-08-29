// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "jinzzaMenuGameMode.generated.h"

/** GameMode for Lvl_MainMenu: no pawn, just a UI-only player controller. */
UCLASS()
class JINZZA_API AjinzzaMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AjinzzaMenuGameMode();
};
