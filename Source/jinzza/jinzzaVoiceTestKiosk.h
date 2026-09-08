// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaInteractableKiosk.h"
#include "jinzzaVoiceTestKiosk.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UjinzzaVoiceTestWidget;

/**
 * A physical, walk-up-to prop for the exhibition test level (Lvl_test) that opens the same
 * UjinzzaVoiceTestWidget the main menu's bottom-right panel uses - both are just separate
 * instances of the same self-contained widget, nothing kiosk-specific lives on the widget class
 * itself. Mirrors AjinzzaWardrobeKiosk's shape exactly (see that class).
 */
UCLASS()
class JINZZA_API AjinzzaVoiceTestKiosk : public AjinzzaInteractableKiosk
{
	GENERATED_BODY()

public:
	AjinzzaVoiceTestKiosk();

	/** Widget class to show. Defaults to UjinzzaVoiceTestWidget if left unset. */
	UPROPERTY(EditAnywhere, Category = "Interaction")
	TSubclassOf<UjinzzaVoiceTestWidget> VoiceTestWidgetClass;

	virtual FText GetInteractionPrompt() const override { return FText::FromString(TEXT("Press E - Voice Test")); }

	/** Opens the voice test panel locally for Interactor (a no-op for anything but the interactor's own client). */
	virtual void Interact(APlayerController* Interactor) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY()
	TObjectPtr<UjinzzaVoiceTestWidget> ActiveWidget;
};
