// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaEmoteTypes.h"
#include "jinzzaEmoteWheelWidget.generated.h"

/**
 * Radial emote-select overlay shown while E is held (see AjinzzaCharacter::DoOpenEmoteWheel).
 * Tracks the mouse cursor's position relative to screen center every tick and reports which of
 * the four quadrants (Up/Down/Left/Right) it's currently over; AjinzzaCharacter reads
 * GetHoveredEmote() when E is released, plays that emote, and closes this widget.
 *
 * UMG-authored: unlike this project's other widgets, this one needs no BindWidget children - lay
 * out four quadrant icons around a circle in the Widget Blueprint (e.g. WBP_EmoteWheel) however
 * you like, matching the same Up/Down/Left/Right regions this class computes (an even split
 * around screen center: Up/Down/Left/Right by whichever axis the cursor deviates from center
 * more). Override BP_OnHoveredEmoteChanged to highlight the icon under the cursor.
 */
UCLASS()
class JINZZA_API UjinzzaEmoteWheelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Emote")
	EJinzzaEmoteType GetHoveredEmote() const { return HoveredEmote; }

protected:
	/** Fired only when the hovered quadrant changes - override in the Widget Blueprint to move a highlight. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Emote")
	void BP_OnHoveredEmoteChanged(EJinzzaEmoteType NewHoveredEmote);

private:
	EJinzzaEmoteType HoveredEmote = EJinzzaEmoteType::None;
};
