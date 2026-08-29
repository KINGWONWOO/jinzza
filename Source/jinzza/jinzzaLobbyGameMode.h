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
};
