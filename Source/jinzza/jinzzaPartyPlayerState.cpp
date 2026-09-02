// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaPartyPlayerState.h"
#include "jinzzaDisguiseComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"

void AjinzzaPartyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AjinzzaPartyPlayerState, bIsGhost);
	DOREPLIFETIME(AjinzzaPartyPlayerState, FaceType);
	DOREPLIFETIME(AjinzzaPartyPlayerState, VoiceFilter);
}

void AjinzzaPartyPlayerState::ServerSetGhost(bool bNewGhost)
{
	if (!HasAuthority())
	{
		return;
	}
	bIsGhost = bNewGhost;
	OnRep_DisguiseChanged();
}

void AjinzzaPartyPlayerState::ServerSetFaceType(EJinzzaFaceType NewFaceType)
{
	if (!HasAuthority())
	{
		return;
	}
	FaceType = NewFaceType;
	OnRep_DisguiseChanged();
}

void AjinzzaPartyPlayerState::ServerSetVoiceFilter(EJinzzaVoiceFilter NewVoiceFilter)
{
	if (!HasAuthority())
	{
		return;
	}
	VoiceFilter = NewVoiceFilter;
	OnRep_DisguiseChanged();
}

void AjinzzaPartyPlayerState::OnRep_DisguiseChanged()
{
	OnDisguiseChanged.Broadcast();

	// Reused as the manual "notify" path from the Server* setters too (both the listen-server's
	// own RepNotify-less local change and every client's RepNotify end up here), so push straight
	// to the owning pawn's component rather than relying on every caller to bind the delegate.
	if (APawn* Pawn = GetPawn())
	{
		if (UjinzzaDisguiseComponent* Disguise = Pawn->FindComponentByClass<UjinzzaDisguiseComponent>())
		{
			Disguise->RefreshDisguise();
		}
	}
}
