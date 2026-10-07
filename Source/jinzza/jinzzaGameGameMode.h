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
	int32 NextUserAliasNumber = 1;
	double LastJoinTime = 0.0;
	TSet<TWeakObjectPtr<APlayerController>> LoadedPlayers;
	FTimerHandle MatchStartTimeoutHandle;

	TWeakObjectPtr<APawn> SeatedJudgePawn;
	TWeakObjectPtr<APawn> SeatedCandidatePawn;
};
