// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaInteractableKiosk.h"
#include "jinzzaStartMatchKiosk.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * A physical, walk-up-to prop placed in Lvl_Lobby that server-travels everyone to Lvl_Game.
 * Host-only - a non-host pressing E here is a no-op (mirrors the old UjinzzaLobbyWidget Start
 * Match button's PC->HasAuthority() check exactly, just relocated). No panel to open - unlike
 * the other kiosks, interacting with this one takes effect immediately.
 */
UCLASS()
class JINZZA_API AjinzzaStartMatchKiosk : public AjinzzaInteractableKiosk
{
	GENERATED_BODY()

public:
	AjinzzaStartMatchKiosk();

	virtual FText GetInteractionPrompt() const override { return FText::FromString(TEXT("Press E - Start Match")); }

	/** Host-only: server-travels everyone to Lvl_Game. No-op for anything but the host's own local controller. */
	virtual void Interact(APlayerController* Interactor) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UTextRenderComponent> Label;
};
