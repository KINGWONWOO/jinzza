// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaPlayerController.h"
#include "jinzzaRoundTypes.h"
#include "jinzzaQuestionTypes.h"
#include "jinzzaGamePlayerController.generated.h"

class UUserWidget;
class APlayerState;
class UAudioComponent;
class ACameraActor;
class UjinzzaSpeakTurnWidget;
class UjinzzaVoteWidget;
class UjinzzaQuestionWidget;

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

	/** Question Time: the Judge's question (UjinzzaQuestionWidget) -> AjinzzaGameGameMode::HandleQuestionSubmitted. */
	UFUNCTION(Server, Reliable)
	void Server_SubmitQuestion(const FString& Text);

	/** Question Time: this answerer's drawing (UjinzzaQuestionWidget) -> AjinzzaGameGameMode::HandleDrawingSubmitted. */
	UFUNCTION(Server, Reliable)
	void Server_SubmitDrawing(const FJinzzaDrawing& Drawing);

	/** Question Time reveal: Answerer's drawing, sent to everyone as their board flips round. Stored on their
	 * AjinzzaPartyPlayerState, where the in-world board picks it up. */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveAnswer(APlayerState* Answerer, const FJinzzaDrawing& Drawing);

	/** Judge ballot (UjinzzaVoteWidget) -> AjinzzaGameGameMode::HandleVote. */
	UFUNCTION(Server, Reliable)
	void Server_CastVote(APlayerState* Candidate);

	/** Local: called once by UjinzzaLoadingScreenSubsystem when this player has loaded the match (level,
	 * preload list, own pawn) - tells the server, which starts the round once everyone has. */
	void ReportLoadComplete();

	/** Server -> this player only: they were the Rank-th (1-based) to finish loading the match. Never
	 * broadcast - UserN numbers follow load order, so others' ranks would give the aliases away. */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveLoadRank(int32 Rank);

	/** Local: this player's load rank for the loading screen, 0 until the server has answered. */
	int32 GetLoadRank() const { return LoadRank; }

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

	/** The real match's extras: the host's End Game overlay and the match music. AjinzzaTestPlayerController
	 * (Lvl_test) turns them off - it plays like the rest of the test level, no cursor. */
	virtual bool UsesMatchExtras() const { return true; }

private:
	UFUNCTION(Server, Reliable)
	void Server_ReportLoaded();

	/** Local: applies the replicated turn - nobody moves; everyone but the speaker watches the speaker
	 * through TurnCamera; any open chat line of a non-speaker is closed. */
	void UpdateSpeakTurn();

	/** Local: nobody moves during a speaking turn or Question Time (seated players can still look around -
	 * AjinzzaCharacter keeps their body facing the seat). Re-asserted every frame - a possession
	 * (ClientRestart) resets the engine's ignore-input counters. */
	void UpdateInputLocks();

	/** Local: shows the cursor while the ask box / drawing screen / ballot needs it, and puts input back the
	 * way this level plays afterwards. (In the match the cursor is always on, so this only matters in Lvl_test.) */
	void UpdateUICursor();
	bool bUICursorWanted = false;

	UPROPERTY(Transient)
	TObjectPtr<UjinzzaSpeakTurnWidget> SpeakTurnWidget;

	UPROPERTY(Transient)
	TObjectPtr<UjinzzaVoteWidget> VoteWidget;

	UPROPERTY(Transient)
	TObjectPtr<UjinzzaQuestionWidget> QuestionWidget;

	/** Local-only camera the audience watches the speaker through (never replicated). */
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> TurnCamera;

	int32 AppliedTurnSerial = 0;
	bool bAppliedAsSpeaker = false;
	bool bLoadReported = false;
	int32 LoadRank = 0;
	bool bTurnMovementLocked = false;
	bool bWasQuestionTime = false;
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
