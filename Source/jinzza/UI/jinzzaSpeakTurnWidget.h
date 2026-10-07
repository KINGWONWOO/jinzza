// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaSpeakTurnWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UWidget;

/**
 * Match HUD for spotlight turns and the Judge vote (created by AjinzzaGamePlayerController):
 *  - Top banner: "Self-introduction 2/5 - User2 is speaking - 14s", "Your turn!", "User3 got the most
 *    votes - final argument", "The Judge is voting...".
 *  - Speech bubbles: during a turn the speaker's chat lines show as bubbles instead of the hand-held
 *    board - your own lines on the right (yellow), the speaker's lines on everyone else's left (white,
 *    with their name). Cleared when the turn ends; nothing is kept.
 *
 * Refreshed every frame by the controller (a collapsed widget doesn't tick on its own).
 */
UCLASS()
class JINZZA_API UjinzzaSpeakTurnWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	void Refresh();

	/** A turn speaker's chat line. bOwn = this player wrote it (right side), else left side. */
	void AddBubble(const FString& SpeakerName, const FString& Text, bool bOwn);

	void ClearBubbles();

private:
	void BuildWidgetTree();
	UWidget* MakeBubble(const FString& SpeakerName, const FString& Text, bool bOwn);

	UPROPERTY()
	TObjectPtr<UWidget> Banner;

	UPROPERTY()
	TObjectPtr<UTextBlock> BannerText;

	UPROPERTY()
	TObjectPtr<UVerticalBox> LeftBubbles;

	UPROPERTY()
	TObjectPtr<UVerticalBox> RightBubbles;

	int32 ShownTurnSerial = -1;
};
