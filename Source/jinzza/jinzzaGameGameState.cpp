// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGameGameState.h"
#include "Net/UnrealNetwork.h"

void AjinzzaGameGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AjinzzaGameGameState, CurrentPhase);
	DOREPLIFETIME(AjinzzaGameGameState, PhaseEndServerTime);
	DOREPLIFETIME(AjinzzaGameGameState, MidEvaluationsRemaining);
	DOREPLIFETIME(AjinzzaGameGameState, LoadedPlayerCount);
	DOREPLIFETIME(AjinzzaGameGameState, ExpectedPlayerCount);
	DOREPLIFETIME(AjinzzaGameGameState, bAllPlayersLoaded);
	DOREPLIFETIME(AjinzzaGameGameState, SpeakTurn);
	DOREPLIFETIME(AjinzzaGameGameState, bVoteOpen);
	DOREPLIFETIME(AjinzzaGameGameState, VotePhase);
	DOREPLIFETIME(AjinzzaGameGameState, VoteEndServerTime);
	DOREPLIFETIME(AjinzzaGameGameState, QuestionState);
}

float AjinzzaGameGameState::GetSpeakTurnTimeRemaining() const
{
	return IsSpeakTurnActive() ? FMath::Max(0.f, static_cast<float>(SpeakTurn.EndServerTime - GetServerWorldTimeSeconds())) : 0.f;
}

float AjinzzaGameGameState::GetVoteTimeRemaining() const
{
	return bVoteOpen ? FMath::Max(0.f, static_cast<float>(VoteEndServerTime - GetServerWorldTimeSeconds())) : 0.f;
}

float AjinzzaGameGameState::GetQuestionStepTimeRemaining() const
{
	return IsQuestionTimeActive() ? FMath::Max(0.f, static_cast<float>(QuestionState.StepEndServerTime - GetServerWorldTimeSeconds())) : 0.f;
}

void AjinzzaGameGameState::ServerSetQuestionState(const FJinzzaQuestionState& NewState)
{
	QuestionState = NewState;
	ForceNetUpdate();
}

void AjinzzaGameGameState::ServerSetSpeakTurn(const FJinzzaSpeakTurn& NewTurn)
{
	SpeakTurn = NewTurn;
}

void AjinzzaGameGameState::ServerSetVote(bool bOpen, EJinzzaRoundPhase Phase, double EndServerTime)
{
	bVoteOpen = bOpen;
	VotePhase = Phase;
	VoteEndServerTime = EndServerTime;
}

void AjinzzaGameGameState::ServerSetLoadProgress(int32 InLoaded, int32 InExpected, bool bInAllLoaded)
{
	LoadedPlayerCount = InLoaded;
	ExpectedPlayerCount = InExpected;
	bAllPlayersLoaded = bInAllLoaded;
}

float AjinzzaGameGameState::GetPhaseTimeRemaining() const
{
	return FMath::Max(0.f, static_cast<float>(PhaseEndServerTime - GetServerWorldTimeSeconds()));
}

void AjinzzaGameGameState::ServerSetPhase(EJinzzaRoundPhase NewPhase, float DurationSeconds, int32 InMidEvaluationsRemaining)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentPhase = NewPhase;
	PhaseEndServerTime = GetServerWorldTimeSeconds() + DurationSeconds;
	MidEvaluationsRemaining = InMidEvaluationsRemaining;
	ForceNetUpdate();
	OnRep_CurrentPhase();
}

void AjinzzaGameGameState::OnRep_CurrentPhase()
{
	OnPhaseChanged.Broadcast(CurrentPhase);
}
