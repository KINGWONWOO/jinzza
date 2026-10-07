// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "jinzzaChatWidget.generated.h"

class UEditableTextBox;

/**
 * The chat input line (bottom-left, created by AjinzzaPlayerController). There is no chat log: what you
 * type is written on your character's hand-held board (UjinzzaChatBoardComponent) and shown to the
 * players around you when you send it.
 *
 * Enter opens the line and raises the board (UI-only input, so typing never moves the character or
 * presses E/Space); each keystroke updates your own side of the board; Enter sends (the board flips to
 * face everyone); ESC or clicking away cancels (the board is lowered).
 *
 * TEMP C++-built like the other jinzza widgets (see docs/umg_widget_authoring_guide.md).
 */
UCLASS()
class JINZZA_API UjinzzaChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	/** Shows the input line and focuses it. The owning controller switches input to UI-only. */
	void OpenInput();

	bool IsInputOpen() const { return bInputOpen; }

protected:
	UFUNCTION()
	void HandleTextChanged(const FText& Text);

	UFUNCTION()
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

private:
	void BuildWidgetTree();
	void CloseInput();

	UPROPERTY()
	TObjectPtr<UEditableTextBox> InputBox;

	bool bInputOpen = false;
};
