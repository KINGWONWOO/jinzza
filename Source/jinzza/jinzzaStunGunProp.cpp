// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaStunGunProp.h"
#include "jinzzaCharacter.h"
#include "Kismet/KismetSystemLibrary.h"

AjinzzaStunGunProp::AjinzzaStunGunProp()
{
	// Hold left click = keep zapping: one zap on the press, then another every half second while it stays down.
	// Each zap re-stuns for StunDuration, so the target stays locked (and the particles keep going on them and
	// on the gun's head) until the button is released, then recovers StunDuration later.
	HoldRepeatInterval = 0.5f;
}

void AjinzzaStunGunProp::OnPropActivated_Implementation()
{
	APawn* Wielder = GetHoldingPawn();
	if (!Wielder)
	{
		return;
	}

	const FVector TraceStart = Wielder->GetActorLocation();
	const FVector TraceEnd = TraceStart + (Wielder->GetActorForwardVector() * Range);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Wielder);
	ActorsToIgnore.Add(this);

	TArray<FHitResult> Hits;
	UKismetSystemLibrary::SphereTraceMulti(
		this,
		TraceStart,
		TraceEnd,
		Radius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		Hits,
		true);

	for (const FHitResult& Hit : Hits)
	{
		if (AjinzzaCharacter* HitCharacter = Cast<AjinzzaCharacter>(Hit.GetActor()))
		{
			HitCharacter->Stun(StunDuration);
		}
	}
}
