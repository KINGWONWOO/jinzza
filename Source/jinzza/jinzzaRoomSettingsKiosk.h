// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaRoomSettingsKiosk.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UUserWidget;

/**
 * A physical, walk-up-to prop placed in Lvl_Lobby that opens the room settings panel
 * (UjinzzaRoomSettingsWidget) when interacted with. Replaces the old pre-create Host Setup
 * screen: Host Game now creates a room with default settings immediately, and the host
 * adjusts them here, live, once everyone's already in the lobby.
 *
 * Proximity is polled by AjinzzaLobbyPlayerController (see its NearbyKiosk/CheckForNearbyKiosk)
 * rather than driven by component overlap events, since the lobby's plain ADefaultPawn has no
 * collision set up for that - polling a distance check on a short timer is simpler and avoids
 * touching the pawn's collision presets.
 */
UCLASS()
class JINZZA_API AjinzzaRoomSettingsKiosk : public AActor
{
	GENERATED_BODY()

public:
	AjinzzaRoomSettingsKiosk();

	/** How close a pawn needs to be (in cm) for AjinzzaLobbyPlayerController to consider this kiosk "nearby". */
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionRadius = 220.f;

	FText GetInteractionPrompt() const { return FText::FromString(TEXT("Press E - Room Settings")); }

	/** Opens the room settings panel locally for Interactor (a no-op for anything but the interactor's own client). */
	void Interact(APlayerController* Interactor);

private:
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveWidget;
};
