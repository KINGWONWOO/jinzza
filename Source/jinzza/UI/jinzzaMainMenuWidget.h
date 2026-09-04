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
class UjinzzaSettingsWidget;
class UAudioComponent;

/**
 * Main menu UI: a button-list page plus a Settings popup-page swapped in via a
 * UWidgetSwitcher.
 *
 * UMG-authored: every property below must exist in this class's Widget Blueprint (e.g.
 * WBP_MainMenu), named exactly as below, for BindWidget to find it. SettingsWidget must be
 * a "User Widget" placed as the Switcher's second child, with its class set to the Settings
 * screen's own Widget Blueprint (e.g. WBP_Settings).
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
	void OnQuitClicked();

private:
	void ShowButtonsPage();
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

	/** Looping menu BGM, started in NativeOnInitialized and stopped in NativeDestruct. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;

	float FadeInElapsed = 0.f;

	FDelegateHandle SessionStatusHandle;
};
