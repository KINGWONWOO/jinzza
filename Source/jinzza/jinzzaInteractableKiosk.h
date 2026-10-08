// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaInteractableKiosk.generated.h"

class APlayerController;
class APawn;
class UUserWidget;
class UjinzzaFaceCameraComponent;

/**
 * Common base for every walk-up-to lobby kiosk (AjinzzaRoomSettingsKiosk, AjinzzaWardrobeKiosk,
 * AjinzzaFriendInviteKiosk, AjinzzaStartMatchKiosk, ...) that AjinzzaLobbyPlayerController polls
 * for proximity and opens with E. Split out of AjinzzaRoomSettingsKiosk once a second kiosk type
 * (Wardrobe) needed the exact same polling/prompt/interact shape - see
 * AjinzzaLobbyPlayerController::CheckForNearbyKiosk.
 *
 * Every kiosk also gets a UjinzzaFaceCameraComponent, so its text labels always turn to face the
 * local player, and is outlined white while it's the player's nearby kiosk (JinzzaHighlight).
 */
UCLASS(Abstract)
class JINZZA_API AjinzzaInteractableKiosk : public AActor
{
	GENERATED_BODY()

public:
	AjinzzaInteractableKiosk();

	/** How close a pawn needs to be (in cm) for AjinzzaLobbyPlayerController to consider this kiosk "nearby". */
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionRadius = 220.f;

	virtual FText GetInteractionPrompt() const PURE_VIRTUAL(AjinzzaInteractableKiosk::GetInteractionPrompt, return FText::GetEmpty(););

	/** Opens this kiosk's panel locally for Interactor (a no-op for anything but the interactor's own client). */
	virtual void Interact(APlayerController* Interactor) PURE_VIRTUAL(AjinzzaInteractableKiosk::Interact, );

	/** The kiosk Pawn is close enough to use (closest one wins), or null. Shared by every controller that lets
	 * players walk up to kiosks (AjinzzaLobbyPlayerController, AjinzzaTestPlayerController). */
	static AjinzzaInteractableKiosk* FindNearby(const APawn* Pawn);

protected:
	/**
	 * Switches Interactor into UI-interactable mode: cursor visible, Game+UI input, keyboard
	 * focus on Widget. Call right after adding a kiosk panel to the viewport.
	 *
	 * Every kiosk used to only ever set bShowMouseCursor = true and never anything else - the
	 * lobby's default input mode (set once, in AjinzzaLobbyPlayerController::BeginPlay) was the
	 * only thing actually keeping the cursor usable, and nothing ever turned it back off after
	 * closing a panel. Fixed 2026-09-07 by making the lobby default to hidden-cursor/Game-only
	 * input instead, and having every kiosk explicitly enter/exit UI mode around its own panel.
	 */
	static void EnterKioskUIMode(APlayerController* Interactor, UUserWidget* Widget);

	/** Restores Interactor to normal hidden-cursor/Game-only input. Call when a kiosk panel closes
	 * (e.g. from its widget's OnNativeDestruct or an explicit "back"/"close" delegate). */
	static void ExitKioskUIMode(APlayerController* Interactor);

	/** Keeps this kiosk's labels facing the local player - see UjinzzaFaceCameraComponent. */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UjinzzaFaceCameraComponent> FaceCamera;
};
