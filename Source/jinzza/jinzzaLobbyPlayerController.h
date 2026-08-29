// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "jinzzaLobbyPlayerController.generated.h"

class UUserWidget;

/** Spawns and displays the lobby widget (player count + host-only Start Match). */
UCLASS()
class JINZZA_API AjinzzaLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> LobbyWidget;
};
