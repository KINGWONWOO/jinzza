// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaInteractableKiosk.generated.h"

/**
 * Common base for every walk-up-to lobby kiosk (AjinzzaRoomSettingsKiosk, AjinzzaWardrobeKiosk,
 * ...) that AjinzzaLobbyPlayerController polls for proximity and opens with E. Split out of
 * AjinzzaRoomSettingsKiosk once a second kiosk type (Wardrobe) needed the exact same
 * polling/prompt/interact shape - see AjinzzaLobbyPlayerController::CheckForNearbyKiosk.
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
};
