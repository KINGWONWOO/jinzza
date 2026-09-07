// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaInteractableKiosk.generated.h"

class APlayerController;
class UUserWidget;

/**
 * Common base for every walk-up-to lobby kiosk (AjinzzaRoomSettingsKiosk, AjinzzaWardrobeKiosk,
 * AjinzzaFriendInviteKiosk, AjinzzaStartMatchKiosk, ...) that AjinzzaLobbyPlayerController polls
 * for proximity and opens with E. Split out of AjinzzaRoomSettingsKiosk once a second kiosk type
 * (Wardrobe) needed the exact same polling/prompt/interact shape - see
 * AjinzzaLobbyPlayerController::CheckForNearbyKiosk.
 */
UCLASS(Abstract)
class JINZZA_API AjinzzaInteractableKiosk : public AActor
{
	GENERATED_BODY()

public:
	/** How close a pawn needs to be (in cm) for AjinzzaLobbyPlayerController to consider this kiosk "nearby". */
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionRadius = 220.f;

	virtual FText GetInteractionPrompt() const PURE_VIRTUAL(AjinzzaInteractableKiosk::GetInteractionPrompt, return FText::GetEmpty(););

	/** Opens this kiosk's panel locally for Interactor (a no-op for anything but the interactor's own client). */
	virtual void Interact(APlayerController* Interactor) PURE_VIRTUAL(AjinzzaInteractableKiosk::Interact, );

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
};
