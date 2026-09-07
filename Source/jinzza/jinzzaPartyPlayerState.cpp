// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaPartyPlayerState.h"
#include "jinzzaDisguiseComponent.h"
#include "jinzzaCharacter.h"
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

	if (bIsGhost)
	{
		// "위장 해제" - clear the underlying disguise data itself, not just have
		// UjinzzaDisguiseComponent visually ignore it while ghosted.
		FaceType = EJinzzaFaceType::None;
		VoiceFilter = EJinzzaVoiceFilter::None;

		// A ghost can no longer hold/use props (AjinzzaCharacter::IsGhost gates new pickups) -
		// this clears whatever was already in hand at the moment of elimination.
		if (AjinzzaCharacter* Character = Cast<AjinzzaCharacter>(GetPawn()))
		{
			Character->ServerForceDropHeldProp();
		}
	}

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
