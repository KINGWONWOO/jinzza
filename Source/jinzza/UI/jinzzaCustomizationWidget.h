// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaCustomizationTypes.h"
#include "jinzzaCustomizationWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;

/**
 * The ONE customization screen, opened from two places: the main menu's Customization button
 * (embedded as a UjinzzaMainMenuWidget switcher page) and AjinzzaWardrobeKiosk in the lobby
 * (as a popup overlay). Both read/write the exact same UjinzzaGameUserSettings fields, so a
 * choice made in either place is the same choice everywhere - there is deliberately only one
 * widget class and one data source, not two parallel customization flows.
 *
 * Five rows (Head/HairColor/Top/Eyebrows/Eyes), each a Prev/Next pair cycling through
 * EJinzzaCustomizationStyle's/EJinzzaHairColor's few temporary placeholder items (see
 * jinzzaCustomizationTypes.h - there's no real per-item art yet, so each row just shows the
 * current option's name as text; HairColorSwatch is the one exception, tinted with the actual
 * selected color since that's real data even without art). OnBackRequested fires on Done/Back;
 * the main menu binds it to switch back to its buttons page, AjinzzaWardrobeKiosk binds it to
 * remove this widget from the viewport - see UjinzzaSettingsWidget for the identical pattern.
 *
 * UMG-authored: add, per row, a UTextBlock value label and two UButtons (Prev/Next) named
 * exactly as below, plus a UImage "HairColorSwatch" and a "DoneButton". All BindWidgetOptional
 * so the class compiles before that layout exists.
 */
UCLASS()
class JINZZA_API UjinzzaCustomizationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	FSimpleMulticastDelegate OnBackRequested;

protected:
	UFUNCTION() void OnHeadPrevClicked();
	UFUNCTION() void OnHeadNextClicked();
	UFUNCTION() void OnHairColorPrevClicked();
	UFUNCTION() void OnHairColorNextClicked();
	UFUNCTION() void OnTopPrevClicked();
	UFUNCTION() void OnTopNextClicked();
	UFUNCTION() void OnEyebrowsPrevClicked();
	UFUNCTION() void OnEyebrowsNextClicked();
	UFUNCTION() void OnEyesPrevClicked();
	UFUNCTION() void OnEyesNextClicked();
	UFUNCTION() void OnDoneClicked();

private:
	void RefreshAllRows();
	static FText GetStyleDisplayName(EJinzzaCustomizationStyle Style);
	static FText GetHairColorDisplayName(EJinzzaHairColor Color);
	static FLinearColor GetHairColorSwatchColor(EJinzzaHairColor Color);

	/** Applies live to the local player's pawn (if any - see UjinzzaCharacterCustomizationComponent) and saves to disk. */
	void CommitChange();

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HeadValueText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> HeadPrevButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> HeadNextButton;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HairColorValueText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> HairColorSwatch;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> HairColorPrevButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> HairColorNextButton;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TopValueText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> TopPrevButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> TopNextButton;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> EyebrowsValueText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> EyebrowsPrevButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> EyebrowsNextButton;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> EyesValueText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> EyesPrevButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> EyesNextButton;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DoneButton;
};
