// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "jinzzaChatTypes.h"
#include "jinzzaPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UUserWidget;
class UjinzzaPauseMenuWidget;
class UjinzzaChatWidget;
class UjinzzaChatBoardComponent;
class UWidget;

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 *
 *  Also owns the local mic (proximity voice - see UjinzzaProximityVoiceComponent for the playback side):
 *  transmits while the push-to-talk key is held, or all the time in Open Mic mode (Settings > Audio), and
 *  never while this player is a ghost.
 */
UCLASS(abstract, config="Game")
class JINZZA_API AjinzzaPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	AjinzzaPlayerController();

	/** Rebuilds the runtime mapping contexts from the current key rebinds, so a rebind in the Settings screen
	 *  applies immediately instead of on the next level load. */
	void RefreshKeyBindings();

	/** Re-reads Push to Talk / Open Mic from settings and starts/stops transmitting to match. */
	void ApplyMicInputMode();

	/** Puts input back the way this level plays once an overlay (ESC menu, chat line, emote wheel, held-prop
	 * panel) closes. Default: game-only, no cursor; the match keeps a cursor (AjinzzaGamePlayerController). */
	virtual void RestoreGameplayInputMode();

	/** True while the loading screen covers the view - the ESC menu and chat stay closed then. */
	bool IsLoadingScreenUp() const;

	/** Opens the in-game ESC menu (UjinzzaPauseMenuWidget), or closes it if it's already open. */
	void TogglePauseMenu();

	void ClosePauseMenu();

	bool IsPauseMenuOpen() const { return PauseMenuWidget != nullptr; }

	/** Chat board: the server cleans up Text (trim, MaxMessageLength, rate limit) and flips this player's
	 * board (UjinzzaChatBoardComponent) to show it to everyone around. Nothing is logged anywhere. */
	UFUNCTION(Server, Reliable)
	void Server_SendChatMessage(const FString& Text);

	/** Chat input opened (true: board raised, blank side out) or cancelled (false: board lowered). */
	UFUNCTION(Server, Reliable)
	void Server_SetChatWriting(bool bWriting);

	/** Local: what's being typed, drawn on our own side of the board. */
	void UpdateChatPreview(const FString& Text);

	/** Called by UjinzzaChatWidget when its input line opens/closes: UI-only input focused on FocusTarget, so
	 * typing never reaches the character, then back to this level's normal input. */
	void EnterChatInputMode(UWidget* FocusTarget);
	void ExitChatInputMode();

	/** True while this player's mic is transmitting. */
	UFUNCTION(BlueprintPure, Category = "Voice")
	bool IsTransmittingVoice() const { return bTransmittingVoice; }

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	/** Duplicates Source and applies any per-player key overrides from UjinzzaGameUserSettings onto the copy,
	 *  so rebinding a key never mutates the shared .uasset Input Mapping Context. */
	UInputMappingContext* BuildRuntimeMappingContext(UInputMappingContext* Source);

	/** Runtime copies handed to the Enhanced Input subsystem in place of DefaultMappingContexts/MobileExcludedMappingContexts. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputMappingContext>> RuntimeMappingContexts;

	/** The engine calls this on login (and after seamless travel) with the server's push-to-talk default,
	 *  resetting the mic - the local Mic Input Mode setting decides instead. */
	virtual void ClientEnableNetworkVoice_Implementation(bool bEnable) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Turn rules (overridden in the match by AjinzzaGamePlayerController): */
	/** Local: true while this player may not talk (e.g. someone else's speaking turn). */
	virtual bool IsVoiceBlocked() const { return false; }
	/** Local: false while this player may not chat (e.g. someone else's speaking turn). */
	virtual bool CanUseChat() const { return true; }
	/** Server: false while chat must not use the hand-held board (a speaking turn shows speech bubbles instead). */
	virtual bool ShouldUseChatBoard() const { return true; }
	/** Server: takes Clean (already validated) somewhere other than the board and returns true, or returns false. */
	virtual bool RouteChatMessage(const FString& Clean) { return false; }

	/** Closes the chat input line if it's open (lowering the board). */
	void CancelChatInput();

	/** False while something else owns the screen (e.g. a lobby kiosk panel) - ESC does nothing then. */
	virtual bool CanOpenPauseMenu() const { return true; }

private:
	/** Adds every runtime mapping context (DefaultMappingContexts, MobileExcludedMappingContexts, voice) to the local player. */
	void AddRuntimeMappingContexts();

	/** Push-to-talk has no .uasset - created here, named JinzzaInput::GetPushToTalkActionName() so the Settings
	 *  screen can rebind it like any other action. */
	void CreatePushToTalkAction();

	/** The ESC menu action has no .uasset either - created here, mapped to Escape / gamepad Start. Not rebindable. */
	void CreatePauseMenuAction();

	void OnPauseMenuPressed();

	/** Enter: opens the chat input line (not while the ESC menu or a kiosk panel is up). */
	void OnChatPressed();

	/** The possessed character's chat board, if it has one. */
	UjinzzaChatBoardComponent* GetChatBoard() const;

	void OnPushToTalkPressed();
	void OnPushToTalkReleased();

	/** Starts/stops transmitting to match push-to-talk, the mic mode setting and ghost state. */
	void UpdateVoiceTransmission();

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PushToTalkAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PauseMenuAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ChatAction;

	UPROPERTY(Transient)
	TObjectPtr<UjinzzaChatWidget> ChatWidget;

	/** Server-side rate limit for Server_SendChatMessage. */
	double LastChatMessageTime = -1000.0;

	/** Created on open, destroyed on close, so its settings page always loads the current values. */
	UPROPERTY(Transient)
	TObjectPtr<UjinzzaPauseMenuWidget> PauseMenuWidget;

	bool bPushToTalkHeld = false;
	bool bTransmittingVoice = false;

	/** Set once the engine has finished setting up voice for this connection (ClientEnableNetworkVoice) - starting
	 *  to talk before that silently does nothing. */
	bool bVoiceReady = false;

	/** Re-checks ghost state (which can change without any input) a few times a second. */
	FTimerHandle VoiceUpdateTimerHandle;
};
