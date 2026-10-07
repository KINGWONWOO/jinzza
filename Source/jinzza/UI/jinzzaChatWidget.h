// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "jinzzaChatTypes.h"
#include "jinzzaChatWidget.generated.h"

class UBorder;
class UScrollBox;
class UEditableTextBox;

/**
 * Text chat, bottom-left of the screen (lobby and match - created by AjinzzaPlayerController).
 *
 * Enter opens the input line (UI-only input, so typing never moves the character or presses E/Space);
 * Enter again sends, ESC or clicking away cancels. Recent lines stay visible for a few seconds, then
 * fade out until the next message or until the input is opened again.
 *
 * Sender names come from the server (AjinzzaPartyPlayerState::GetDisplayName): nicknames in the lobby,
 * "User1".."UserN" / "Judge" in the match. Ghost lines are grey with a [Ghost] tag and only reach ghosts.
 *
 * TEMP C++-built like the other jinzza widgets (see docs/umg_widget_authoring_guide.md).
 */
UCLASS()
class JINZZA_API UjinzzaChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void AddMessage(const FJinzzaChatMessage& Message);

	/** Shows the input line and focuses it. The owning controller switches input to UI-only. */
	void OpenInput();

	bool IsInputOpen() const { return bInputOpen; }

protected:
	UFUNCTION()
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

private:
	void BuildWidgetTree();
	void CloseInput();

	UPROPERTY()
	TObjectPtr<UBorder> Panel;

	UPROPERTY()
	TObjectPtr<UScrollBox> MessageList;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> InputBox;

	bool bInputOpen = false;
	double LastActivityTime = -1000.0;
};
