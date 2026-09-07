// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaPlayerController.h"
#include "jinzzaLobbyPlayerController.generated.h"

class UUserWidget;
class AjinzzaInteractableKiosk;

/**
 * Spawns and displays the lobby widget (player count + host-only Start Match), and polls for
 * a nearby AjinzzaInteractableKiosk (AjinzzaRoomSettingsKiosk, AjinzzaWardrobeKiosk, ...) so the
 * player can press E to open it.
 *
 * Derives from AjinzzaPlayerController (not the bare engine APlayerController) specifically so
 * its SetupInputComponent() actually adds DefaultMappingContexts/MobileExcludedMappingContexts
 * to the Enhanced Input subsystem - it used to derive straight from APlayerController, which
 * meant Lvl_Lobby never had any Enhanced Input mapping context installed at all (WASD/Look/Jump
 * were all silently dead), confirmed 2026-09-07.
 */
UCLASS()
class JINZZA_API AjinzzaLobbyPlayerController : public AjinzzaPlayerController
{
	GENERATED_BODY()

public:
	AjinzzaLobbyPlayerController();

	/** Widget class to show. Defaults to UjinzzaLobbyWidget if left unset (WBP_Lobby if it exists, else the raw C++ class). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> LobbyWidgetClass;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void CheckForNearbyKiosk();
	void OnInteractPressed();

	UPROPERTY()
	TObjectPtr<UUserWidget> LobbyWidget;

	UPROPERTY()
	TObjectPtr<AjinzzaInteractableKiosk> NearbyKiosk;

	FTimerHandle KioskCheckTimerHandle;
};
