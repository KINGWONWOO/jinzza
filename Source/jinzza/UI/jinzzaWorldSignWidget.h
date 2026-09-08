// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaWorldSignWidget.generated.h"

class UTextBlock;

/**
 * Minimal UMG wrapper around a single UTextBlock, used to render world-space signage via a
 * UWidgetComponent (AjinzzaWorldSignActor) instead of UTextRenderComponent.
 *
 * UTextRenderComponent cannot render this project's only Korean font (SacheonUju-Regular_Font)
 * at all - it has FontCacheType = Runtime, and TextRenderComponent.cpp's
 * FTextRenderSceneProxy::CreateRenderThreadResources bails out immediately for any Runtime-cached
 * font ("Runtime fonts can't currently be used here as they use the font cache from Slate
 * application which can only be used on the game thread"), building zero mesh. Slate/UMG (what
 * this widget uses) is exactly that font cache, so it renders this font fine.
 *
 * UMG-authored: add a single UTextBlock named exactly "SignText" to the Widget Blueprint (e.g.
 * WBP_WorldSign) that subclasses this, with its Font set to SacheonUju-Regular_Font and
 * Justification centered. SignText is BindWidgetOptional so this class still compiles before
 * that layout exists.
 */
UCLASS()
class JINZZA_API UjinzzaWorldSignWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Sign")
	void SetSignText(const FText& Text);

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SignText;
};
