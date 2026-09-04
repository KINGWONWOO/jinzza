// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaInteractableKiosk.h"
#include "jinzzaWardrobeKiosk.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UjinzzaCustomizationWidget;

/**
 * A physical, walk-up-to prop placed in Lvl_Lobby that opens the same UjinzzaCustomizationWidget
 * the main menu's Customization button opens - both read/write the same
 * UjinzzaGameUserSettings fields, so picking an item here or on the main menu shows up in the
 * other. Mirrors AjinzzaRoomSettingsKiosk's shape exactly (see that class).
 */
UCLASS()
class JINZZA_API AjinzzaWardrobeKiosk : public AjinzzaInteractableKiosk
{
	GENERATED_BODY()

public:
	AjinzzaWardrobeKiosk();

	/** Widget class to show. Defaults to UjinzzaCustomizationWidget if left unset (WBP_Customization if it exists, else the raw C++ class). */
	UPROPERTY(EditAnywhere, Category = "Interaction")
	TSubclassOf<UjinzzaCustomizationWidget> CustomizationWidgetClass;

	virtual FText GetInteractionPrompt() const override { return FText::FromString(TEXT("Press E - Wardrobe")); }

	/** Opens the customization panel locally for Interactor (a no-op for anything but the interactor's own client). */
	virtual void Interact(APlayerController* Interactor) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY()
	TObjectPtr<UjinzzaCustomizationWidget> ActiveWidget;
};
