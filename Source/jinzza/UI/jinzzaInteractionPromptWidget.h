// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaInteractionPromptWidget.generated.h"

class UTextBlock;

/**
 * Small "[F] Pick Up" style prompt shown above an AjinzzaInteractableProp while the local
 * player is looking at it (see AjinzzaCharacter::UpdateInteractionFocus). Hosted on a
 * screen-space UWidgetComponent (AjinzzaInteractableProp::InteractionPromptComponent), so it
 * always faces the viewer without any billboard logic of its own.
 *
 * TEMP C++-built (see docs/umg_widget_authoring_guide.md): builds its own tree in
 * BuildWidgetTree() (a note-style panel from JinzzaUI::MakeNoteBackground around a centered
 * PromptText) instead of relying on a Designer-authored WBP_InteractionPrompt layout with a real
 * key-icon Image - this project's noob-game reference F_Prompt icon is still only an unfetched
 * Git LFS pointer (see PROJECT_STATUS.md). When real icon art exists, delete BuildWidgetTree(),
 * restore `meta = (BindWidgetOptional)` on PromptText, and add the icon Image in the Designer.
 */
UCLASS()
class JINZZA_API UjinzzaInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	/** Sets the prompt label (e.g. "Pick Up" or "Use" - see AjinzzaInteractableProp::InteractPromptText). */
	UFUNCTION(BlueprintCallable, Category = "Prompt")
	void SetPrompt(const FText& Text);

private:
	void BuildWidgetTree();

	UPROPERTY()
	TObjectPtr<UTextBlock> PromptText;
};
