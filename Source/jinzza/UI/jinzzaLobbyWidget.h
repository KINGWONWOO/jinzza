// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaLobbyWidget.generated.h"

class UTextBlock;
class UWidget;
class UAudioComponent;

/**
 * Pre-match lobby UI: a pure display HUD showing the replicated match settings and connected
 * player count, plus the bottom-center "Press E" interaction prompt for whichever kiosk the
 * player is standing near.
 *
 * No buttons live here anymore - Invite Friends and Start Match both moved to walk-up-to kiosks
 * (AjinzzaFriendInviteKiosk / AjinzzaStartMatchKiosk) specifically so the lobby can default to
 * hidden-cursor/Game-only input (WASD look/move work immediately) instead of needing an
 * always-visible, always-clickable cursor just to reach two buttons - see
 * AjinzzaLobbyPlayerController::BeginPlay.
 *
 * TEMP C++-built (see docs/umg_widget_authoring_guide.md): builds its own tree in
 * BuildWidgetTree() instead of relying on a Designer-authored WBP_Lobby layout, so PIE isn't
 * blocked while the visual design pass hasn't happened yet. When that pass happens, delete
 * BuildWidgetTree(), restore `meta = (BindWidget)` on every property below, and lay them out
 * for real in WBP_Lobby's Designer per the guide.
 */
UCLASS()
class JINZZA_API UjinzzaLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Shows a bottom-center interaction prompt (e.g. "Press E - Room Settings"), or hides it if PromptText is empty. */
	void SetInteractionPrompt(const FText& PromptText);

private:
	void BuildWidgetTree();

	/** Hides IntroOverlay - called once from a timer set in NativeOnInitialized so the welcome
	 * panel shown when the lobby first opens goes away on its own after a few seconds. */
	void HideIntro();

	UPROPERTY()
	TObjectPtr<UTextBlock> SettingsText;

	UPROPERTY()
	TObjectPtr<UTextBlock> PlayerCountText;

	UPROPERTY()
	TObjectPtr<UTextBlock> InteractPromptText;

	/** Looping lobby BGM, started in NativeOnInitialized and stopped in NativeDestruct. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;

	/** Full-screen welcome overlay shown when the lobby first opens, auto-hidden after a few
	 * seconds by a timer set in NativeOnInitialized - see BuildWidgetTree/HideIntro. */
	UPROPERTY()
	TObjectPtr<UWidget> IntroOverlay;

	FTimerHandle IntroTimerHandle;
};
