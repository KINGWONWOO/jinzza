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
 * UMG-authored: add an Image showing the interact key (this project's noob-game reference
 * assets have a matching F_Prompt icon, currently only present there as an unfetched Git LFS
 * pointer - see PROJECT_STATUS.md) plus a UTextBlock named exactly "PromptText" to the Widget
 * Blueprint (e.g. WBP_InteractionPrompt) that subclasses this. PromptText is BindWidgetOptional
 * so the class still compiles before that layout exists.
 */
UCLASS()
class JINZZA_API UjinzzaInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Sets the prompt label (e.g. "Pick Up" or "Use" - see AjinzzaInteractableProp::InteractPromptText). */
	UFUNCTION(BlueprintCallable, Category = "Prompt")
	void SetPrompt(const FText& Text);

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText;
};
