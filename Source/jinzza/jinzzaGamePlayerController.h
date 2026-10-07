// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaPlayerController.h"
#include "jinzzaRoundTypes.h"
#include "jinzzaGamePlayerController.generated.h"

class UUserWidget;
class APlayerState;
class UAudioComponent;
class ACameraActor;
class UjinzzaSpeakTurnWidget;
class UjinzzaVoteWidget;

/**
 * Spawns the minimal in-round overlay (host-only End Game button) for Lvl_Game, and receives
 * this player's private role assignment - see AjinzzaGameGameMode::AssignRoles().
 *
 * Derives from AjinzzaPlayerController (not the bare engine APlayerController) specifically so
 * its inherited SetupInputComponent() actually adds DefaultMappingContexts/
 * MobileExcludedMappingContexts to the Enhanced Input subsystem - it used to derive straight
 * from APlayerController, which meant Lvl_Game never had any Enhanced Input mapping context
 * installed at all (WASD/Look/Jump were all silently dead), confirmed 2026-09-07.
 */
UCLASS()
class JINZZA_API AjinzzaGamePlayerController : public AjinzzaPlayerController
{
	GENERATED_BODY()

public:
	AjinzzaGamePlayerController();

	/**
	 * Server-only: tells this player their role and, for Imitators, who the Real One is (nullptr
	 * for every other role). Not replicated further - each client only ever learns what the
	 * design doc says they're allowed to know (see AjinzzaPartyPlayerState's class comment).
	 */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveRoleAssignment(EJinzzaPartyRole InRole, APlayerState* InRealOne);

	// Named GetLocalPartyRole (not GetLocalRole) - AActor already declares a GetLocalRole() that
	// returns ENetRole (network role), and UHT rejects a UFUNCTION override with different
	// parameters/return type under that name.
	UFUNCTION(BlueprintPure, Category = "Party")
	EJinzzaPartyRole GetLocalPartyRole() const { return LocalRole; }

	/** Only meaningful when GetLocalPartyRole() == Imitator. */
	UFUNCTION(BlueprintPure, Category = "Party")
	APlayerState* GetKnownRealOne() const { return KnownRealOne; }

	/** A turn speaker's chat line, shown as a speech bubble (see UjinzzaSpeakTurnWidget). */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveTurnMessage(APlayerState* Speaker, const FString& Text);

	/** Judge ballot (UjinzzaVoteWidget) -> AjinzzaGameGameMode::HandleVote. */
	UFUNCTION(Server, Reliable)
	void Server_CastVote(APlayerState* Candidate);

	/** Local: called once by UjinzzaLoadingScreenSubsystem when this player has loaded the match (level,
	 * preload list, own pawn) - tells the server, which starts the round once everyone has. */
	void ReportLoadComplete();

	/** Widget class to show. Defaults to UjinzzaGameEndWidget if left unset (WBP_GameEnd if it exists, else the raw C++ class). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> GameEndWidgetClass;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;

	// Speaking-turn rules (AjinzzaGameGameState::GetSpeakTurn): only the speaker talks / chats, and their
	// chat goes to speech bubbles instead of the board.
	virtual bool IsVoiceBlocked() const override;
	virtual bool CanUseChat() const override;
	virtual bool ShouldUseChatBoard() const override;
	virtual bool RouteChatMessage(const FString& Clean) override;

	/** Back to the cursor-visible game-and-UI mode BeginPlay sets up for the End Game overlay. */
	virtual void RestoreGameplayInputMode() override;

private:
	UFUNCTION(Server, Reliable)
	void Server_ReportLoaded();

	/** Local: applies the replicated turn - nobody moves; everyone but the speaker watches the speaker
	 * through TurnCamera; any open chat line of a non-speaker is closed. */
	void UpdateSpeakTurn();

	UPROPERTY(Transient)
	TObjectPtr<UjinzzaSpeakTurnWidget> SpeakTurnWidget;

	UPROPERTY(Transient)
	TObjectPtr<UjinzzaVoteWidget> VoteWidget;

	/** Local-only camera the audience watches the speaker through (never replicated). */
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> TurnCamera;

	int32 AppliedTurnSerial = 0;
	bool bTurnMovementLocked = false;
	bool bWatchingTurnCamera = false;

	UPROPERTY()
	TObjectPtr<UUserWidget> GameEndWidget;

	/** Looping in-round BGM, started in BeginPlay and stopped in EndPlay - same TEMP-placeholder
	 * pattern as the main menu/lobby BGM (see UjinzzaMainMenuWidget/UjinzzaLobbyWidget). Local-
	 * controller-only, like the rest of this class's BeginPlay. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;

	EJinzzaPartyRole LocalRole = EJinzzaPartyRole::None;

	UPROPERTY()
	TObjectPtr<APlayerState> KnownRealOne;

	/** Plays a one-shot notification sound on every round-phase transition - see BeginPlay/EndPlay. */
	void HandlePhaseChanged(EJinzzaRoundPhase NewPhase);
	FDelegateHandle PhaseChangedHandle;
};
