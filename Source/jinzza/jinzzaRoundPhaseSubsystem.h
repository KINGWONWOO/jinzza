// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "jinzzaRoundTypes.h"
#include "jinzzaRoundPhaseSubsystem.generated.h"

class AjinzzaGameGameState;
struct FJinzzaMatchSettings;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnJinzzaServerPhaseEntered, EJinzzaRoundPhase /*NewPhase*/);

/**
 * Server-authoritative driver for the 8-phase round state machine (design doc section 6). Runs
 * only on the server/listen-host - StartRound() no-ops on clients, which only ever observe phase
 * state via the replicated AjinzzaGameGameState. A plain UGameInstanceSubsystem as named in the
 * design doc's C++ class list (section 11): it holds no per-actor state, just timing/sequencing
 * logic plus a single FTimerHandle, and writes its results into AjinzzaGameGameState for
 * replication.
 *
 * Mid-evaluation looping: the design doc (section 2) specifies how many mid-evaluations a lobby
 * gets (1-3, read here from FJinzzaMatchSettings::VoteCount) but doesn't fully spec the phase
 * flow for more than one (section 17 lists this as still-undecided). This implementation's
 * extrapolation: repeat MidEvaluation -> FreeTime2 as a pair, VoteCount times, before proceeding
 * to Interview - the VoteCount=1 case collapses to exactly the doc's worked 6-player example.
 */
UCLASS()
class JINZZA_API UjinzzaRoundPhaseSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Server-only. Resets to RoleAssignment and begins advancing. No-op if this instance isn't authoritative. */
	void StartRound();

	UFUNCTION(BlueprintPure, Category = "Round")
	EJinzzaRoundPhase GetCurrentPhase() const { return CurrentPhase; }

	/**
	 * Server-side hook for gameplay systems (role assignment, future vote/interview managers) to
	 * react to phase entry. Not replicated - only ever fires where StartRound() actually runs.
	 * Clients should read AjinzzaGameGameState::OnPhaseChanged instead.
	 */
	FOnJinzzaServerPhaseEntered OnServerPhaseEntered;

private:
	void EnterPhase(EJinzzaRoundPhase NewPhase);
	void AdvancePhase();

	float GetPhaseDurationSeconds(EJinzzaRoundPhase Phase) const;
	int32 GetTotalPlayerCount() const;
	int32 GetCandidateCount() const;
	AjinzzaGameGameState* GetJinzzaGameState() const;
	FJinzzaMatchSettings GetMatchSettings() const;

	EJinzzaRoundPhase CurrentPhase = EJinzzaRoundPhase::None;

	/** Total mid-evaluations this round should run (fixed from lobby settings when RoleAssignment starts). */
	int32 TotalMidEvaluations = 1;

	/** How many MidEvaluation phases have completed so far - drives the FreeTime2<->MidEvaluation loop. */
	int32 MidEvaluationsCompleted = 0;

	FTimerHandle PhaseTimerHandle;
};
