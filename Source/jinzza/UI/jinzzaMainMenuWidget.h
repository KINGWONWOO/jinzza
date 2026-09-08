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
 * Main menu UI: a button-list page plus Settings/Customization popup-pages swapped in via a
 * UWidgetSwitcher, a live character preview render, and a voice-modification test panel.
 *
 * TEMP C++-built (see docs/umg_widget_authoring_guide.md): builds its own tree in
 * BuildWidgetTree() instead of relying on a Designer-authored WBP_MainMenu layout, so PIE isn't
 * blocked while the visual design pass hasn't happened yet. SettingsWidget and
 * CustomizationWidget are both constructed directly as nested widget instances (which build their
 * own trees the same way) rather than loading WBP_Settings/WBP_Customization classes.
 * CustomizationButton sits on the right side of the button page. A "LOGO" text badge stands in
 * for the company logo in the top-left corner until the user supplies the real image - swap it
 * for a UImage at that point. CharacterPreviewImage and VoiceTestWidget are still left
 * unconstructed (null) since their backing content (AjinzzaCharacterPreviewCapture wiring /
 * WBP_VoiceTest) doesn't exist yet - existing code already null-checks both. When the visual
 * design pass happens, delete BuildWidgetTree(), restore `meta = (BindWidget)`/
 * `BindWidgetOptional` on the properties below, and lay them out for real in WBP_MainMenu's
 * Designer per the guide (including placing CharacterPreviewImage on the right and
 * VoiceTestWidget at the bottom-right once that content exists).
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

	/** Opens the Customization switcher page - see UjinzzaCustomizationWidget (shared with AjinzzaWardrobeKiosk in the lobby). Placed on the right side of the button page - see BuildWidgetTree. */
	UPROPERTY()
	TObjectPtr<UButton> CustomizationButton;

	UPROPERTY()
	TObjectPtr<UjinzzaCustomizationWidget> CustomizationWidget;

	/** Shows AjinzzaCharacterPreviewCapture's render target - a temporary character preview on the right side of the menu. Left null - see class comment. */
	UPROPERTY()
	TObjectPtr<UImage> CharacterPreviewImage;

	/** Bottom-right voice-modification test panel (speak into the mic, hear it filtered back) - see UjinzzaVoiceTestWidget. Left null - see class comment. */
	UPROPERTY()
	TObjectPtr<UjinzzaVoiceTestWidget> VoiceTestWidget;

	/** Looping menu BGM, started in NativeOnInitialized and stopped in NativeDestruct. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;

	float FadeInElapsed = 0.f;
	bool bCharacterPreviewWired = false;

	FDelegateHandle SessionStatusHandle;
};
