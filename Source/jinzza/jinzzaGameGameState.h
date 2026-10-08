// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "jinzzaRoundTypes.h"
#include "jinzzaQuestionTypes.h"
#include "jinzzaGameGameState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnJinzzaRoundPhaseChanged, EJinzzaRoundPhase /*NewPhase*/);

class APlayerState;

/** The current spotlight speaking turn (self-introduction or final argument). Replicated as one unit. */
USTRUCT(BlueprintType)
struct FJinzzaSpeakTurn
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EJinzzaSpeakTurnKind Kind = EJinzzaSpeakTurnKind::None;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerState> Speaker;

	/** Server world time the turn ends at. */
	UPROPERTY(BlueprintReadOnly)
	double EndServerTime = 0.0;

	/** 1-based position in the speaking order, and its length (1/1 for a final argument). */
	UPROPERTY(BlueprintReadOnly)
	int32 Number = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Total = 0;

	/** Where every listener's camera looks from while the speaker talks (computed by the server, so all agree). */
	UPROPERTY(BlueprintReadOnly)
	FVector CameraLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FRotator CameraRotation = FRotator::ZeroRotator;

	/** Bumped for every new turn, so back-to-back turns are told apart even if replication merges them. */
	UPROPERTY()
	int32 Serial = 0;
};

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

	/** Current spotlight turn - Kind is None between turns. See AjinzzaGameGameMode's turn system. */
	const FJinzzaSpeakTurn& GetSpeakTurn() const { return SpeakTurn; }
	bool IsSpeakTurnActive() const { return SpeakTurn.Kind != EJinzzaSpeakTurnKind::None; }
	bool IsTurnSpeaker(const APlayerState* PlayerState) const { return IsSpeakTurnActive() && PlayerState && SpeakTurn.Speaker == PlayerState; }
	float GetSpeakTurnTimeRemaining() const;

	/** Judge vote (mid-evaluation / final decision) - open while the Judge picks, see AjinzzaGameGameMode::OpenVote. */
	bool IsVoteOpen() const { return bVoteOpen; }
	EJinzzaRoundPhase GetVotePhase() const { return VotePhase; }
	float GetVoteTimeRemaining() const;

	/** Server-only. */
	void ServerSetSpeakTurn(const FJinzzaSpeakTurn& NewTurn);
	void ServerSetVote(bool bOpen, EJinzzaRoundPhase Phase, double EndServerTime);

	/** Question Time cycle (see FJinzzaQuestionState / AjinzzaGameGameMode's question flow). Step is None outside it. */
	const FJinzzaQuestionState& GetQuestionState() const { return QuestionState; }
	bool IsQuestionTimeActive() const { return QuestionState.IsActive(); }
	float GetQuestionStepTimeRemaining() const;

	/** Server-only. */
	void ServerSetQuestionState(const FJinzzaQuestionState& NewState);

	/** Match start gate (see AjinzzaGameGameMode::NotifyPlayerLoaded): true once every player has loaded
	 * the level. Each player's loading screen stays up until then, so everyone sees the match together. */
	bool AreAllPlayersLoaded() const { return bAllPlayersLoaded; }
	int32 GetLoadedPlayerCount() const { return LoadedPlayerCount; }
	int32 GetExpectedPlayerCount() const { return ExpectedPlayerCount; }

	/** Server-only. */
	void ServerSetLoadProgress(int32 InLoaded, int32 InExpected, bool bInAllLoaded);

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

	UPROPERTY(Replicated)
	int32 LoadedPlayerCount = 0;

	UPROPERTY(Replicated)
	int32 ExpectedPlayerCount = 0;

	UPROPERTY(Replicated)
	bool bAllPlayersLoaded = false;

	UPROPERTY(Replicated)
	FJinzzaSpeakTurn SpeakTurn;

	UPROPERTY(Replicated)
	bool bVoteOpen = false;

	UPROPERTY(Replicated)
	EJinzzaRoundPhase VotePhase = EJinzzaRoundPhase::None;

	UPROPERTY(Replicated)
	double VoteEndServerTime = 0.0;

	UPROPERTY(Replicated)
	FJinzzaQuestionState QuestionState;
};
