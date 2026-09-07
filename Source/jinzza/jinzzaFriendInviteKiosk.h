// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaInteractableKiosk.h"
#include "jinzzaFriendInviteKiosk.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UjinzzaFriendInviteWidget;

/**
 * A physical, walk-up-to prop placed in Lvl_Lobby that opens the centered Invite Friends panel
 * (UjinzzaFriendInviteWidget) - replaces the old always-on-screen Invite Friends button in
 * UjinzzaLobbyWidget so the lobby's default input stays hidden-cursor/Game-only. Mirrors
 * AjinzzaRoomSettingsKiosk's shape exactly (see that class).
 */
UCLASS()
class JINZZA_API AjinzzaFriendInviteKiosk : public AjinzzaInteractableKiosk
{
	GENERATED_BODY()

public:
	AjinzzaFriendInviteKiosk();

	/** Widget class to show. Defaults to UjinzzaFriendInviteWidget if left unset (WBP_FriendInvite if it exists, else the raw C++ class). */
	UPROPERTY(EditAnywhere, Category = "Interaction")
	TSubclassOf<UjinzzaFriendInviteWidget> FriendInviteWidgetClass;

	virtual FText GetInteractionPrompt() const override { return FText::FromString(TEXT("Press E - Invite Friends")); }

	/** Opens the friend invite panel locally for Interactor (a no-op for anything but the interactor's own client). */
	virtual void Interact(APlayerController* Interactor) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY()
	TObjectPtr<UjinzzaFriendInviteWidget> ActiveWidget;
};
