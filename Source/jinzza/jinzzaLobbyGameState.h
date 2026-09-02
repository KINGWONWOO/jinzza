// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "jinzzaMatchSettings.h"
#include "jinzzaLobbyGameState.generated.h"

/** Replicates the host's chosen match settings to every client in Lvl_Lobby. */
UCLASS()
class JINZZA_API AjinzzaLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated, BlueprintReadOnly)
	FJinzzaMatchSettings MatchSettings;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
