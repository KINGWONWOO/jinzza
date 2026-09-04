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
 * UMG-authored: every property below must exist in this class's Widget Blueprint (e.g.
 * WBP_MainMenu), named exactly as below, for BindWidget to find it. SettingsWidget/
 * CustomizationWidget must each be a "User Widget" placed as one of the Switcher's children,
 * with its class set to that screen's own Widget Blueprint (e.g. WBP_Settings/
 * WBP_Customization). CustomizationButton, CharacterPreviewImage, and VoiceTestWidget are
 * BindWidgetOptional (added after this class already had a hand-authored layout) - place
 * CharacterPreviewImage on the right and VoiceTestWidget (a WBP_VoiceTest instance) at the
 * bottom-right; see AjinzzaCharacterPreviewCapture for what CharacterPreviewImage's texture
 * comes from.
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
	void ShowButtonsPage();
	void TryWireCharacterPreview();
	void HandleSessionStatusChanged(EJinzzaSessionStatus Status, const FString& Message);
	UjinzzaGameInstance* GetJinzzaGameInstance() const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HostButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UjinzzaSettingsWidget> SettingsWidget;

	/** Root panel of the button-list page, faded in on open for a bit of life. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ButtonsPageRoot;

	/** Opens the Customization switcher page - see UjinzzaCustomizationWidget (shared with AjinzzaWardrobeKiosk in the lobby). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CustomizationButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UjinzzaCustomizationWidget> CustomizationWidget;

	/** Shows AjinzzaCharacterPreviewCapture's render target - a temporary character preview on the right side of the menu. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CharacterPreviewImage;

	/** Bottom-right voice-modification test panel (speak into the mic, hear it filtered back) - see UjinzzaVoiceTestWidget. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UjinzzaVoiceTestWidget> VoiceTestWidget;

	/** Looping menu BGM, started in NativeOnInitialized and stopped in NativeDestruct. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;

	float FadeInElapsed = 0.f;
	bool bCharacterPreviewWired = false;

	FDelegateHandle SessionStatusHandle;
};
