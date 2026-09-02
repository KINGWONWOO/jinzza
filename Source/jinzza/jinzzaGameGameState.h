// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "jinzzaRoundTypes.h"
#include "jinzzaGameGameState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnJinzzaRoundPhaseChanged, EJinzzaRoundPhase /*NewPhase*/);

/**
 * Replicated round state for Lvl_Game: current phase, when it ends, and how many
 * mid-evaluations remain. Driven server-side by UjinzzaRoundPhaseSubsystem via ServerSetPhase();
 * this class just holds and replicates the result, matching the design doc's split between
 * APartyGameState (replicated state, section 11) and the subsystem (transition logic).
 */
UCLASS()
class JINZZA_API AjinzzaGameGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Round")
	EJinzzaRoundPhase GetCurrentPhase() const { return CurrentPhase; }

	/** Seconds remaining in the current phase, computed against the (already clock-synced) server time. 0 once the phase has ended or for phases with no timer (e.g. RoundComplete). */
	UFUNCTION(BlueprintPure, Category = "Round")
	float GetPhaseTimeRemaining() const;

	/** How many more MidEvaluation phases this round will run, including the current one if CurrentPhase == MidEvaluation. */
	UFUNCTION(BlueprintPure, Category = "Round")
	int32 GetMidEvaluationsRemaining() const { return MidEvaluationsRemaining; }

	/** Server-only: called by UjinzzaRoundPhaseSubsystem when it advances the phase. */
	void ServerSetPhase(EJinzzaRoundPhase NewPhase, float DurationSeconds, int32 InMidEvaluationsRemaining);

	/** Broadcast on both server and clients whenever CurrentPhase changes. */
	FOnJinzzaRoundPhaseChanged OnPhaseChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_CurrentPhase();

	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase)
	EJinzzaRoundPhase CurrentPhase = EJinzzaRoundPhase::None;

	/** Server world time (GetServerWorldTimeSeconds()) at which the current phase ends. */
	UPROPERTY(Replicated)
	double PhaseEndServerTime = 0.0;

	UPROPERTY(Replicated)
	int32 MidEvaluationsRemaining = 0;
};
