// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaGameInstance.h"
#include "jinzzaMainMenuWidget.generated.h"

class UTextBlock;
class UButton;
class UWidgetSwitcher;
class UWidget;
class UImage;
class UjinzzaSettingsWidget;
class UjinzzaCustomizationWidget;
class UjinzzaVoiceTestWidget;
class UAudioComponent;

/**
 * Main menu UI: a button-list page plus Settings/Customization/VoiceTest popup-pages swapped in
 * via a UWidgetSwitcher, plus a live character preview render.
 *
 * Button-page layout keeps the position split from an earlier round (title/divider/tagline/status
 * top-right; Host/Settings/Quit stacked bottom-left; Customize/Voice Test stacked bottom-right,
 * each column on its own JinzzaUI::MakePanelBackground noir panel), but the VISUAL STYLE was
 * reworked back to JINZZA's own established noir courtroom/interrogation theme (see JinzzaUI's own
 * header comment) after the user asked to match the game's concept rather than keep NOOB-GAME's
 * cute pastel look from earlier rounds: Host/Settings/Quit are the same
 * MakePrimaryButton/MakeSecondaryButton/MakeWarningButton pills every other jinzza screen uses;
 * Customize/Voice Test are circular icon buttons (JinzzaUI::MakeCircleIconButton) with purpose-
 * built icons instead of borrowed NOOB art - a masquerade mask (crimson) for Customize, evoking
 * the "Imitator" disguise premise directly, and a microphone (gold) for Voice Test - both built
 * from plain primitives (JinzzaUI::MakeMaskIcon/MakeMicIcon), not source images. The top-left logo
 * badge uses the same mask icon in place of the old "LOGO" text placeholder.
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
	void TryWireCharacterPreview();
	void HandleSessionStatusChanged(EJinzzaSessionStatus Status, const FString& Message);
	UjinzzaGameInstance* GetJinzzaGameInstance() const;

	UPROPERTY()
	TObjectPtr<UWidgetSwitcher> Switcher;

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

	/** Looping menu BGM, started in NativeOnInitialized and stopped in NativeDestruct. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;

	float FadeInElapsed = 0.f;
	bool bCharacterPreviewWired = false;

	FDelegateHandle SessionStatusHandle;
};
