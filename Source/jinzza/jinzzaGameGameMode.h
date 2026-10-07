// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "jinzzaRoundTypes.h"
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

private:
	void OnRoundPhaseEntered(EJinzzaRoundPhase NewPhase);
	void AssignRoles();

	/** "Judge" for Judge (may be null), "User1".."UserN" for everyone else in a fresh random order - the
	 * role shuffle puts the Real One first, so its order must not leak into the numbers. */
	void AssignDisplayAliases(const TArray<AjinzzaPartyPlayerState*>& Players, AjinzzaPartyPlayerState* Judge);
	void TryStartRound();

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

	/** Stops any turn/vote left over from the previous phase. */
	void CancelTurnsAndVote();

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
	FTimerHandle MatchStartTimeoutHandle;

	TWeakObjectPtr<APawn> SeatedJudgePawn;
	TWeakObjectPtr<APawn> SeatedCandidatePawn;
};
