// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "jinzzaRoundTypes.h"
#include "jinzzaQuestionTypes.h"
#include "jinzzaGameGameMode.generated.h"

class AjinzzaPartyPlayerState;

/**
 * GameMode for Lvl_Game: drives the round via UjinzzaRoundPhaseSubsystem and assigns roles
 * (AssignRoles()) when the RoleAssignment phase starts.
 */
UCLASS()
class JINZZA_API AjinzzaGameGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AjinzzaGameGameMode();

	/** Server-only. Transitions Target to ghost status (design doc section 6: mid-evaluation
	 * elimination) via AjinzzaPartyPlayerState::ServerSetGhost, which handles the actual changes
	 * (disguise removed, held prop dropped, movement/emotes still allowed). No-op if Target is
	 * already a ghost or this instance isn't authoritative. This is the entry point a future
	 * mid-evaluation vote-tally system (Week 7, not built yet - see AjinzzaGameGameMode's class
	 * list in the design doc) will call once it can actually name an eliminated candidate; nothing
	 * calls it yet, same "ready once a consumer needs it" pattern as
	 * UjinzzaRoundPhaseSubsystem::NotifyPhaseConditionMet. */
	UFUNCTION(BlueprintCallable, Category = "Round")
	void EliminateToGhost(AjinzzaPartyPlayerState* Target);

	/** A player finished loading the match (AjinzzaGamePlayerController::ReportLoadComplete). Once all
	 * players have, AjinzzaGameGameState's gate opens (everyone's loading screen hides together) and
	 * the round starts. */
	void NotifyPlayerLoaded(APlayerController* Player);

	/** Server: a Judge's vote (AjinzzaGamePlayerController::Server_CastVote). Changes their vote if they already
	 * voted. Ignored unless a vote is open, Voter is a Judge and Target a living candidate. */
	void HandleVote(APlayerController* Voter, APlayerState* Target);

	/** Server: the turn speaker's chat line - sent to everyone as a speech bubble (no board, no log). */
	void BroadcastTurnMessage(APlayerState* Speaker, const FString& Text);

	/** Server: the Judge's question (AjinzzaGamePlayerController::Server_SubmitQuestion). Only while Asking. */
	void HandleQuestionSubmitted(APlayerController* Asker, const FString& Text);

	/** Server: an answerer's drawing (AjinzzaGamePlayerController::Server_SubmitDrawing). Only while Answering,
	 * once per cycle. Kept here until the Showing step reveals it. */
	void HandleDrawingSubmitted(APlayerController* Answerer, FJinzzaDrawing Drawing);

	// Question Time timings (seconds), per cycle: the doc's 20 s question / 15 s answer / 30 s discussion,
	// plus the question sign's drop-in and a short grace for the last drawings to arrive.
	static constexpr float QuestionAskSeconds = 20.f;
	static constexpr float QuestionRevealSeconds = 3.f;
	static constexpr float QuestionAnswerSeconds = 15.f;
	static constexpr float QuestionAnswerGraceSeconds = 0.75f;
	static constexpr float QuestionShowSeconds = 30.f;
	/** Answer boards flip round one after another, this far apart. */
	static constexpr float QuestionBoardFlipInterval = 0.8f;
	static constexpr float QuestionCycleSeconds = QuestionAskSeconds + QuestionRevealSeconds + QuestionAnswerSeconds + QuestionAnswerGraceSeconds + QuestionShowSeconds;

	// Turn/vote timings (seconds). Self-intro matches UjinzzaRoundPhaseSubsystem's 20 s x candidates phase
	// length; vote + final argument fit inside the 60 s MidEvaluation / FinalDecision phases.
	static constexpr float SelfIntroSecondsPerCandidate = 20.f;
	static constexpr float VoteSeconds = 30.f;
	static constexpr float FinalArgumentSeconds = 20.f;

protected:
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** If the Real One quits mid-match the round can't go on: everyone left is sent back to Lvl_Lobby,
	 * silently. (The host quitting closes the server, so its clients end up on the title screen instead -
	 * see UjinzzaGameInstance::LeaveToTitle.) */
	virtual void Logout(AController* Exiting) override;

	// Protected rather than private so AjinzzaTestGameMode (Lvl_test) can run single pieces of the match -
	// a self-introduction, one Question Time cycle, a vote - with practice dummies, on demand.

	void OnRoundPhaseEntered(EJinzzaRoundPhase NewPhase);
	void AssignRoles();

	/** "Judge" for Judge (may be null), "User1".."UserN" for everyone else in the order they finished
	 * loading the match (LoadOrder) - independent of the role shuffle, which puts the Real One first. */
	void AssignDisplayAliases(const TArray<AjinzzaPartyPlayerState*>& Players, AjinzzaPartyPlayerState* Judge);
	/** Starts the round once everyone has loaded. AjinzzaTestGameMode never does. */
	virtual void TryStartRound();

	/** Someone who can take part: a connected player, or (Lvl_test) a practice dummy run by an AI controller. */
	static bool IsPresent(const APlayerState* PlayerState);

	/** The question/drawing handlers below, by player state (the controller-based ones resolve to these). */
	void SubmitQuestionFrom(APlayerState* Asker, const FString& Text);
	void SubmitAnswerDrawing(APlayerState* Answerer, FJinzzaDrawing Drawing);

	void CheckAllPlayersLoaded();
	/** bForce: the MatchStartTimeoutSeconds safety net - start with whoever made it. */
	void OpenMatchStartGate(bool bForce);
	void OnMatchStartTimeout() { OpenMatchStartGate(true); }

	/** Shows/hides zone geometry and teleports players for NewPhase (design doc section 8-6 /
	 * 13-4's BP_ZoneTeleportTrigger concept, implemented centrally here instead of as per-zone
	 * placed actors). Zones are matched by Actor Tag ("Zone.<Name>", shared by a zone's dressing,
	 * PlayerStarts, and - for Interview - its seat markers), not by class, so no new Blueprint
	 * class is required per zone. */
	void UpdateZoneForPhase(EJinzzaRoundPhase NewPhase);

	static FName GetZoneTagForPhase(EJinzzaRoundPhase Phase);

	// --- Spotlight speaking turns (self-introduction, final argument) ---------------------------------
	// One speaker at a time: moved to the zone's spotlight ("<Zone>.Spotlight"-tagged actor, if placed),
	// everyone else's camera turned on them (from "<Zone>.Camera" if placed, else a spot in front of the
	// speaker), nobody can move, only the speaker's mic is heard, only the speaker can chat - as speech
	// bubbles (AjinzzaGamePlayerController applies all of that on each machine from the replicated turn).

	/** Self-introduction: living candidates in User1..UserN order, SelfIntroSecondsPerCandidate each. */
	void StartSelfIntroductions();
	void AdvanceSelfIntroduction();

	void BeginSpeakTurn(EJinzzaSpeakTurnKind Kind, AjinzzaPartyPlayerState* Speaker, float Seconds, int32 Number, int32 Total, const FString& ZoneTag);
	/** Ends the current turn (if any) and returns the speaker to where they were standing. */
	void EndSpeakTurn();

	// --- Judge vote -> final argument -> elimination (MidEvaluation and FinalDecision) ------------------

	void OpenVote(EJinzzaRoundPhase Phase);
	/** Tallies: most votes wins, ties and no-votes are settled at random. Then the final argument. */
	void CloseVote();
	void StartFinalArgument(AjinzzaPartyPlayerState* Condemned);
	/** After the final argument: eliminate (ghost) the condemned and end the phase. */
	void FinishFinalArgument();

	/** Stops any turn/vote/question cycle left over from the previous phase. */
	void CancelTurnsAndVote();

	// --- Question Time ----------------------------------------------------------------------------------
	// Everyone is seated in Zone.Question (on "Zone.Question.Seat"-tagged markers if placed, else in a row at
	// the zone's PlayerStart) and can't move; each seat gets a camera in front of it for the split screen
	// (UjinzzaQuestionWidget renders them on every machine). Voice stays open throughout. Per cycle:
	// Asking -> Revealing -> Answering -> Showing (see EJinzzaQuestionStep), QuestionTimeCycles times.

	void StartQuestionTime();
	void BeginQuestionCycle();
	void RevealQuestion();
	void BeginAnswering();
	void FinishAnswering();
	void RevealNextAnswerBoard();
	void FinishQuestionCycle();
	/** Lowers the boards, frees the seated players and clears the replicated state. Safe to call any time. */
	void EndQuestionTime();
	void SetQuestionStep(EJinzzaQuestionStep Step, float VisibleSeconds);
	void PushQuestionState();
	/** A seated player left: drop their seat and move on if they were holding things up. */
	void HandleQuestionSeatLeft(APlayerState* Leaving);
	bool AllAnswersIn() const;
	void SetAnswerBoardsUp(bool bUp);
	FString PickFallbackQuestion();
	/** Builds Seats (+ teleports and freezes everyone in them). */
	void SeatQuestionParticipants();

	TArray<AjinzzaPartyPlayerState*> GetLivingCandidates() const;
	int32 CountConnectedJudges() const;

	/** Placeholder stand-in for the real judge-picks-a-candidate targeting system (design doc's
	 * "1대1 면담 대상 지정", decided during Free Time 2 - not built yet, Week 7). Auto-pairs the
	 * Judge with the first non-Judge PartyPlayerState so the interview room/forced-seating is
	 * exercisable today; replace the candidate lookup here once real target designation exists. */
	void EnterInterviewZone();
	void ExitInterviewZone();

	bool bRoundStarted = false;
	bool bReturningToLobby = false;
	FTimerHandle RoundStartGraceTimerHandle;

	/** Players the lobby sent here (UjinzzaGameInstance::ConsumeExpectedMatchPlayers); 0 when the level
	 * was opened directly (PIE), in which case a short no-new-joins grace period stands in for it. */
	int32 ExpectedPlayers = 0;
	bool bAllPlayersLoaded = false;

	bool bAliasesAssigned = false;

	TArray<TWeakObjectPtr<AjinzzaPartyPlayerState>> SelfIntroOrder;
	int32 SelfIntroIndex = INDEX_NONE;
	int32 SpeakTurnSerial = 0;
	FTimerHandle SpeakTurnTimerHandle;
	TWeakObjectPtr<APawn> SpotlightPawn;
	FVector SpotlightReturnLocation = FVector::ZeroVector;
	FRotator SpotlightReturnRotation = FRotator::ZeroRotator;

	EJinzzaRoundPhase VotePhase = EJinzzaRoundPhase::None;
	TMap<TWeakObjectPtr<APlayerController>, TWeakObjectPtr<AjinzzaPartyPlayerState>> Votes;
	TWeakObjectPtr<AjinzzaPartyPlayerState> CondemnedPlayer;
	FTimerHandle VoteTimerHandle;
	int32 NextUserAliasNumber = 1;
	double LastJoinTime = 0.0;
	TSet<TWeakObjectPtr<APlayerController>> LoadedPlayers;
	/** Players in the order they reported loading this level - the basis of the UserN numbers. */
	TArray<TWeakObjectPtr<APlayerController>> LoadOrder;
	FTimerHandle MatchStartTimeoutHandle;

	FJinzzaQuestionState QuestionState;
	/** > 0: run this many Question Time cycles instead of the lobby's QuestionTimeCycles setting. */
	int32 QuestionCyclesOverride = 0;
	TMap<TWeakObjectPtr<APlayerState>, FJinzzaDrawing> SubmittedDrawings;
	TArray<TWeakObjectPtr<APawn>> QuestionSeatedPawns;
	TArray<int32> UsedFallbackQuestions;
	int32 NextAnswerBoardToFlip = 0;
	FTimerHandle QuestionTimerHandle;
	FTimerHandle QuestionFlipTimerHandle;

	TWeakObjectPtr<APawn> SeatedJudgePawn;
	TWeakObjectPtr<APawn> SeatedCandidatePawn;
};
