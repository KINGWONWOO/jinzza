// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "jinzzaLobbyPlayerController.generated.h"

class UUserWidget;
class AjinzzaRoomSettingsKiosk;

/**
 * Spawns and displays the lobby widget (player count + host-only Start Match), and polls for
 * a nearby AjinzzaRoomSettingsKiosk so the player can press E to open the room settings panel.
 */
UCLASS()
class JINZZA_API AjinzzaLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void CheckForNearbyKiosk();
	void OnInteractPressed();

	UPROPERTY()
	TObjectPtr<UUserWidget> LobbyWidget;

	UPROPERTY()
	TObjectPtr<AjinzzaRoomSettingsKiosk> NearbyKiosk;

	FTimerHandle KioskCheckTimerHandle;
};
