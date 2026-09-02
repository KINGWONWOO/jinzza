// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyGameState.h"
#include "Net/UnrealNetwork.h"

void AjinzzaLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AjinzzaLobbyGameState, MatchSettings);
}
