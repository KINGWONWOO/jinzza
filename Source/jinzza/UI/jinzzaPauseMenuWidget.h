// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaPauseMenuWidget.generated.h"

class UButton;
class UWidgetSwitcher;
class UjinzzaSettingsWidget;

/**
 * In-game ESC menu (lobby and match - opened by AjinzzaPlayerController::TogglePauseMenu). A small
 * sticker panel centered over the live game, no full-screen backdrop:
 *   Settings        - swaps the panel for the main menu's UjinzzaSettingsWidget in its compact
 *                     (960x640) layout, same Apply/Back behavior as on the title screen.
 *   Quit Game       - leaves the session and returns to the title screen (UjinzzaGameInstance::LeaveToTitle).
 *   Exit to Desktop - closes the game.
 *   Cancel          - closes this menu (so does ESC; ESC on the settings page goes back one step).
 *
 * The game keeps running underneath - it's multiplayer, nothing is actually paused.
 *
 * TEMP C++-built like the other jinzza widgets (see docs/umg_widget_authoring_guide.md).
 */
UCLASS()
class JINZZA_API UjinzzaPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Cancel / ESC - the owning player controller removes the widget and restores game input. */
	FSimpleMulticastDelegate OnCloseRequested;

protected:
	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnQuitGameClicked();

	UFUNCTION()
	void OnExitToDesktopClicked();

	UFUNCTION()
	void OnCancelClicked();

private:
	void BuildWidgetTree();
	void ShowButtonsPage();

	UPROPERTY()
	TObjectPtr<UWidgetSwitcher> Switcher;

	UPROPERTY()
	TObjectPtr<UjinzzaSettingsWidget> SettingsWidget;

	UPROPERTY() TObjectPtr<UButton> SettingsButton;
	UPROPERTY() TObjectPtr<UButton> QuitGameButton;
	UPROPERTY() TObjectPtr<UButton> ExitToDesktopButton;
	UPROPERTY() TObjectPtr<UButton> CancelButton;
};
