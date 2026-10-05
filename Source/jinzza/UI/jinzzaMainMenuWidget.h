// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "jinzzaGameInstance.h"
#include "jinzzaMainMenuWidget.generated.h"

class UTextBlock;
class UButton;
class UWidgetSwitcher;
class UWidget;
class UImage;
class UBorder;
class UjinzzaSettingsWidget;
class UjinzzaCustomizationWidget;
class UjinzzaVoiceTestWidget;
class UAudioComponent;

/**
 * Hover/press feedback for the main menu's "sticker" buttons (see MakeStickerButton in the .cpp):
 * on hover the sticker face fills with its accent color, its text flips to dark and the face lifts
 * a few pixels off its drop shadow; on press it sinks onto the shadow. Owned (kept alive) by
 * UjinzzaMainMenuWidget::StickerFx.
 */
UCLASS()
class UJinzzaMenuStickerFx : public UObject
{
	GENERATED_BODY()

public:
	void Bind(UButton* InButton, UBorder* InFace, const FSlateBrush& InNormalBrush, const FSlateBrush& InHoverBrush);
	void AddText(UTextBlock* Text, const FLinearColor& NormalColor, const FLinearColor& HoverColor);

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();

	UFUNCTION()
	void HandlePressed();

	UFUNCTION()
	void HandleReleased();

private:
	void Apply(bool bHot, float LiftY);

	UPROPERTY()
	TObjectPtr<UBorder> Face;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> Texts;

	TArray<FLinearColor> TextNormalColors;
	TArray<FLinearColor> TextHoverColors;
	FSlateBrush NormalBrush;
	FSlateBrush HoverBrush;
	bool bHovered = false;
};

/**
 * Main menu UI: a button-list page plus Settings/Customization/VoiceTest popup-pages swapped in
 * via a UWidgetSwitcher, plus a live character preview render.
 *
 * Button-page layout: T_Logo image + status text top-right; Host/Settings/Quit stacked
 * bottom-left; Customize/Voice Test stacked bottom-right. The page background is transparent so
 * the 3D menu scene (AjinzzaMenuSceneDirector) shows behind it; the popup pages dim it instead
 * (ShowPage).
 *
 * Visual style matches T_Logo's cartoon "sticker" look: near-black fills, thick white outlines,
 * fully rounded shapes, a solid drop shadow under each sticker, and bright candy colors on hover
 * (UJinzzaMenuStickerFx). Host/Settings/Quit are wide pill stickers with a white round badge icon
 * (plus / lines / cross, drawn from primitives) and a Korean sub-label; Customize/Voice Test are
 * colored round stickers with the mask/mic icons (JinzzaUI::MakeMaskIcon/MakeMicIcon) and a small
 * caption pill. The top-left badge (company-logo placeholder) is a matching rounded-square sticker.
 *
 * TEMP C++-built (see docs/umg_widget_authoring_guide.md): builds its own tree in
 * BuildWidgetTree() instead of relying on a Designer-authored WBP_MainMenu layout, so PIE isn't
 * blocked while the visual design pass hasn't happened yet. SettingsWidget, CustomizationWidget
 * and VoiceTestWidget are all constructed directly as nested widget instances (which build their
 * own trees the same way) rather than loading WBP_Settings/WBP_Customization/WBP_VoiceTest
 * classes. A "LOGO" text badge stands in for the company logo in the top-left corner until the
 * user supplies the real image - swap it for a UImage at that point. CharacterPreviewImage is
 * still left unconstructed (null) since its backing content (AjinzzaCharacterPreviewCapture
 * wiring) doesn't exist yet - existing code already null-checks it. When the visual design pass
 * happens, delete BuildWidgetTree(), restore `meta = (BindWidget)`/`BindWidgetOptional` on the
 * properties below, and lay them out for real in WBP_MainMenu's Designer per the guide.
 *
 * Host Game creates a Steam session immediately with default match settings and travels
 * straight to the lobby - there is no pre-create setup screen. Joining is invite-only: a
 * player accepts a Steam overlay invite (see UjinzzaGameInstance::OnSessionUserInviteAccepted)
 * rather than browsing a room list, so there is no Join button here.
 */
UCLASS()
class JINZZA_API UjinzzaMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UFUNCTION()
	void OnHostClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnCustomizationClicked();

	UFUNCTION()
	void OnVoiceTestClicked();

	UFUNCTION()
	void OnQuitClicked();

private:
	void BuildWidgetTree();
	void ShowButtonsPage();
	/** Switches Switcher to PageIndex and tints Backdrop to match (clear only on the button page). */
	void ShowPage(int32 PageIndex);
	void TryWireCharacterPreview();
	void HandleSessionStatusChanged(EJinzzaSessionStatus Status, const FString& Message);
	UjinzzaGameInstance* GetJinzzaGameInstance() const;

	UPROPERTY()
	TObjectPtr<UWidgetSwitcher> Switcher;

	/** Full-screen root border: transparent over the 3D menu scene on the button page, dimmed behind the popup pages - see ShowPage. */
	UPROPERTY()
	TObjectPtr<UBorder> Backdrop;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY()
	TObjectPtr<UButton> HostButton;

	UPROPERTY()
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY()
	TObjectPtr<UButton> QuitButton;

	UPROPERTY()
	TObjectPtr<UjinzzaSettingsWidget> SettingsWidget;

	/** Root panel of the button-list page, faded in on open for a bit of life. */
	UPROPERTY()
	TObjectPtr<UWidget> ButtonsPageRoot;

	/** Opens the Customization switcher page - see UjinzzaCustomizationWidget (shared with AjinzzaWardrobeKiosk in the lobby). Sits in the bottom-right button column - see BuildWidgetTree. */
	UPROPERTY()
	TObjectPtr<UButton> CustomizationButton;

	UPROPERTY()
	TObjectPtr<UjinzzaCustomizationWidget> CustomizationWidget;

	/** Shows AjinzzaCharacterPreviewCapture's render target - a temporary character preview on the right side of the menu. Left null - see class comment. */
	UPROPERTY()
	TObjectPtr<UImage> CharacterPreviewImage;

	/** Opens the VoiceTest switcher page - sits directly below CustomizationButton in the bottom-right button column, per user request, rather than the old always-visible corner panel. */
	UPROPERTY()
	TObjectPtr<UButton> VoiceTestButton;

	/** Voice-modification test panel (speak into the mic, hear it filtered back), shown as a central switcher page when VoiceTestButton is clicked - see UjinzzaVoiceTestWidget. */
	UPROPERTY()
	TObjectPtr<UjinzzaVoiceTestWidget> VoiceTestWidget;

	/** Hover/press effect objects for the sticker buttons - held here so they aren't garbage collected (delegates only hold weak refs). */
	UPROPERTY()
	TArray<TObjectPtr<UJinzzaMenuStickerFx>> StickerFx;

	/** Looping menu BGM, started in NativeOnInitialized and stopped in NativeDestruct. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;

	float FadeInElapsed = 0.f;
	bool bCharacterPreviewWired = false;

	FDelegateHandle SessionStatusHandle;
};
