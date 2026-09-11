// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaCustomizationTypes.h"
#include "jinzzaCustomizationWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UWidgetSwitcher;
class UVerticalBox;
class AjinzzaCharacterPreviewCapture;

/**
 * The ONE customization screen, opened from two places: the main menu's Customization button
 * (embedded as a UjinzzaMainMenuWidget switcher page) and AjinzzaWardrobeKiosk in the lobby
 * (as a popup overlay). Both read/write the exact same UjinzzaGameUserSettings fields, so a
 * choice made in either place is the same choice everywhere - there is deliberately only one
 * widget class and one data source, not two parallel customization flows.
 *
 * Layout: a live character preview on the left (see below) and, on the right, four tabs -
 * Head (Head style/Eyebrows/Eyes rows), Clothes (Top row), Accessories (Accessory row, new),
 * Colors (Hair Color row + swatch) - each a Prev/Next pair cycling through
 * EJinzzaCustomizationStyle/EJinzzaHairColor/EJinzzaAccessoryStyle's few temporary placeholder
 * items (see jinzzaCustomizationTypes.h - there's no real per-item art yet, so each row just
 * shows the current option's name as text; HairColorSwatch is the one exception, tinted with the
 * actual selected color since that's real data even without art). Any change applies immediately
 * to both the left preview and (if a real pawn already exists) the live character - see
 * CommitChange(). OnBackRequested fires on Done/Back; the main menu binds it to switch back to
 * its buttons page, AjinzzaWardrobeKiosk binds it to remove this widget from the viewport - see
 * UjinzzaSettingsWidget for the identical pattern.
 *
 * The left preview is a spawned AjinzzaCharacterPreviewCapture (see that class) rendering into a
 * runtime render target shown via CharacterPreviewImage - spawned on demand in NativeOnInitialized
 * rather than requiring one hand-placed per level, so it works the same way whether this widget
 * is embedded in the main menu or popped up from the lobby's Wardrobe kiosk. Destroyed in
 * NativeDestruct so repeatedly opening/closing the kiosk popup (which constructs a brand new
 * widget instance each time - see AjinzzaWardrobeKiosk::Interact) doesn't leak preview actors.
 *
 * TEMP C++-built (see docs/umg_widget_authoring_guide.md, same pattern as UjinzzaSettingsWidget/
 * UjinzzaRoomSettingsWidget/etc.): builds its own tree in BuildWidgetTree() since WBP_Customization
 * has never had a Designer-authored layout (there's no unreal-mcp tool that can place child
 * widgets into a WBP's visual tree - see [[unreal-mcp-gotchas]]). Properties below stay
 * BindWidgetOptional (harmless either way) so a future hand-authored WBP_Customization layout
 * would still bind to them instead of BuildWidgetTree()'s constructed ones - see that method's
 * own early-out (`if (WidgetTree->RootWidget) return;`).
 */
UCLASS()
class JINZZA_API UjinzzaCustomizationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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
	UFUNCTION() void OnAccessoryPrevClicked();
	UFUNCTION() void OnAccessoryNextClicked();
	UFUNCTION() void OnDoneClicked();

	UFUNCTION() void OnHeadTabClicked();
	UFUNCTION() void OnClothesTabClicked();
	UFUNCTION() void OnAccessoriesTabClicked();
	UFUNCTION() void OnColorsTabClicked();

private:
	void BuildWidgetTree();
	void RefreshAllRows();
	void ShowTab(int32 TabIndex);
	void RefreshCharacterPreview();
	static FText GetStyleDisplayName(EJinzzaCustomizationStyle Style);
	static FText GetHairColorDisplayName(EJinzzaHairColor Color);
	static FLinearColor GetHairColorSwatchColor(EJinzzaHairColor Color);
	static FText GetAccessoryStyleDisplayName(EJinzzaAccessoryStyle Style);

	/** Applies live to the local player's pawn (if any - see UjinzzaCharacterCustomizationComponent) and the left preview, and saves to disk. */
	void CommitChange();

	// --- Left preview panel ---
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> CharacterPreviewImage;
	UPROPERTY(Transient) TObjectPtr<AjinzzaCharacterPreviewCapture> PreviewCapture;
	bool bCharacterPreviewWired = false;

	// --- Right side: tab bar + pages ---
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> HeadTabButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ClothesTabButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AccessoriesTabButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ColorsTabButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidgetSwitcher> TabSwitcher;

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

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AccessoryValueText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AccessoryPrevButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AccessoryNextButton;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DoneButton;
};
