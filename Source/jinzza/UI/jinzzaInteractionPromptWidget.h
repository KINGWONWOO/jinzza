// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaInteractionPromptWidget.generated.h"

class UTextBlock;

/**
 * Small "[E] Pick Up" style prompt shown above an AjinzzaInteractableProp while the local
 * player is looking at it (see AjinzzaCharacter::UpdateInteractionFocus). Hosted on a
 * screen-space UWidgetComponent (AjinzzaInteractableProp::InteractionPromptComponent), so it
 * always faces the viewer without any billboard logic of its own.
 *
 * C++-built (see docs/umg_widget_authoring_guide.md): a note-style panel with a key cap
 * (JinzzaUI::MakeKeyCap) and the PromptText label. The key cap shows whatever key is bound to
 * IA_Interact right now (JinzzaInput::GetBoundKey), so a rebind in the Settings screen shows up
 * the next time the prompt appears - no per-key icon art needed.
 */
UCLASS()
class JINZZA_API UjinzzaInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	/** Sets the prompt label (e.g. "Pick Up" or "Use" - see AjinzzaInteractableProp::InteractPromptText). */
	UFUNCTION(BlueprintCallable, Category = "Prompt")
	void SetPrompt(const FText& Text);

	/** Re-reads the key bound to IA_Interact into the key cap. Called whenever the prompt is shown. */
	UFUNCTION(BlueprintCallable, Category = "Prompt")
	void RefreshBoundKey();

private:
	void BuildWidgetTree();

	UPROPERTY()
	TObjectPtr<UTextBlock> PromptText;

	UPROPERTY()
	TObjectPtr<UTextBlock> KeyText;
};
