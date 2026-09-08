// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaEmoteTypes.h"
#include "jinzzaEmoteWheelWidget.generated.h"

class UTextBlock;

/**
 * Radial emote-select overlay shown while E is held (see AjinzzaCharacter::DoOpenEmoteWheel).
 * Tracks the mouse cursor's position relative to screen center every tick and reports which of
 * the four quadrants (Up/Down/Left/Right) it's currently over; AjinzzaCharacter reads
 * GetHoveredEmote() when E is released, plays that emote, and closes this widget.
 *
 * TEMP C++-built (see docs/umg_widget_authoring_guide.md, same pattern as UjinzzaSettingsWidget/
 * UjinzzaCustomizationWidget/etc.): builds a plain text-label cross layout (Up/Down/Left/Right)
 * in BuildWidgetTree(), highlighting whichever quadrant is hovered, since this class previously
 * needed no BindWidget children at all - meaning without any Designer-authored WBP_EmoteWheel
 * content, opening the wheel showed literally nothing on screen even though the underlying
 * hover-detection logic worked. BP_OnHoveredEmoteChanged is still fired every change too, so a
 * future hand-authored WBP_EmoteWheel can still hook it for a nicer highlight/icon treatment -
 * this native visual is not exclusive with that.
 */
UCLASS()
class JINZZA_API UjinzzaEmoteWheelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Emote")
	EJinzzaEmoteType GetHoveredEmote() const { return HoveredEmote; }

protected:
	/** Fired only when the hovered quadrant changes - override in the Widget Blueprint to move a highlight. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Emote")
	void BP_OnHoveredEmoteChanged(EJinzzaEmoteType NewHoveredEmote);

private:
	void BuildWidgetTree();

	/** Recolors the four quadrant labels so the hovered one reads as selected. */
	void RefreshHighlight();

	EJinzzaEmoteType HoveredEmote = EJinzzaEmoteType::None;

	UPROPERTY() TObjectPtr<UTextBlock> ThumbsUpLabel;
	UPROPERTY() TObjectPtr<UTextBlock> ThumbsDownLabel;
	UPROPERTY() TObjectPtr<UTextBlock> MiddleFingerLabel;
	UPROPERTY() TObjectPtr<UTextBlock> PointLabel;
};
