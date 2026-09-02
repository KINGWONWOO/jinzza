// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGameGameState.h"
#include "Net/UnrealNetwork.h"

void AjinzzaGameGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AjinzzaGameGameState, CurrentPhase);
	DOREPLIFETIME(AjinzzaGameGameState, PhaseEndServerTime);
	DOREPLIFETIME(AjinzzaGameGameState, MidEvaluationsRemaining);
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
