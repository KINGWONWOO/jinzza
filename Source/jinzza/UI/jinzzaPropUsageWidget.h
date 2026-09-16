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
 * TEMP C++-built (see docs/umg_widget_authoring_guide.md, same pattern as UjinzzaGameEndWidget/
 * UjinzzaInteractionPromptWidget): builds a bottom-right note panel (icon + text row) in
 * BuildWidgetTree() instead of relying on a Designer-authored WBP_PropUsageHUD layout - without
 * it, holding a prop showed no usage hint on screen at all even though ShowPropUsageHUD/
 * SetPropInfo's logic worked. When a real HUD art pass happens, delete BuildWidgetTree(), restore
 * `meta = (BindWidgetOptional)` on both properties, and lay them out for real in the Designer.
 */
UCLASS()
class JINZZA_API UjinzzaPropUsageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	/** Sets the usage icon/description for the currently held prop. Either may be unset. */
	UFUNCTION(BlueprintCallable, Category = "Prop")
	void SetPropInfo(UTexture2D* Icon, const FText& Description);

private:
	void BuildWidgetTree();

	UPROPERTY()
	TObjectPtr<UImage> UsageIcon;

	UPROPERTY()
	TObjectPtr<UTextBlock> UsageText;
};
