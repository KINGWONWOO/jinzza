// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaPropUsageWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

/**
 * Bottom-right HUD element explaining how to use whatever prop the local player is currently
 * holding (e.g. "Left Click: Bounce, Right Click: Throw" for the basketball - see
 * AjinzzaInteractableProp::UsageIcon/UsageDescription). Shown/hidden by
 * AjinzzaCharacter::ShowPropUsageHUD/HidePropUsageHUD, which AjinzzaInteractableProp::
 * OnRep_HoldingPawn calls on whichever locally-controlled character just gained or lost this
 * specific prop.
 *
 * UMG-authored: add a UImage named exactly "UsageIcon" and a UTextBlock named exactly
 * "UsageText" to the Widget Blueprint (e.g. WBP_PropUsageHUD) that subclasses this, anchored to
 * the bottom-right of the canvas. Both are BindWidgetOptional so the class compiles either way.
 */
UCLASS()
class JINZZA_API UjinzzaPropUsageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Sets the usage icon/description for the currently held prop. Either may be unset. */
	UFUNCTION(BlueprintCallable, Category = "Prop")
	void SetPropInfo(UTexture2D* Icon, const FText& Description);

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> UsageIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UsageText;
};
