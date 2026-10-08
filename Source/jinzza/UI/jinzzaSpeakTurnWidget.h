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
 *    board, alternating between the left and right screen edges (1st left, 2nd right, ...) so the
 *    middle stays clear for the speaker. Your own lines are yellow, the speaker's lines white with their
 *    name. Cleared when the turn ends; nothing is kept.
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

	/** A turn speaker's chat line, on the side opposite the previous one. bOwn = this player wrote it (yellow). */
	void AddBubble(const FString& SpeakerName, const FString& Text, bool bOwn);

	void ClearBubbles();

private:
	void BuildWidgetTree();
	UWidget* MakeBubble(const FString& SpeakerName, const FString& Text, bool bOwn, bool bLeft);

	UPROPERTY()
	TObjectPtr<UWidget> Banner;

	UPROPERTY()
	TObjectPtr<UTextBlock> BannerText;

	/** One full-width feed; each bubble's slot hugs the left or right edge. */
	UPROPERTY()
	TObjectPtr<UVerticalBox> Bubbles;

	bool bNextBubbleLeft = true;

	int32 ShownTurnSerial = -1;
};
