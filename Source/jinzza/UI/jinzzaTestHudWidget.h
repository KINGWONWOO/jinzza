// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaTestHudWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * Lvl_test HUD (AjinzzaTestPlayerController): the walk-up kiosk prompt (key cap + what it does, bottom center,
 * same look as the lobby's) and a small "TEST MAP" badge top-left with the controls worth remembering.
 */
UCLASS()
class JINZZA_API UjinzzaTestHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	/** Empty hides the prompt. */
	void SetInteractionPrompt(const FText& PromptText);

private:
	void BuildWidgetTree();

	UPROPERTY()
	TObjectPtr<UWidget> Prompt;

	UPROPERTY()
	TObjectPtr<UTextBlock> PromptKeyText;

	UPROPERTY()
	TObjectPtr<UTextBlock> PromptText;
};
