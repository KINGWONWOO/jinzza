// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaBatProp.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"

void AjinzzaBatProp::OnPropActivated_Implementation()
{
	APawn* Wielder = GetHoldingPawn();
	if (!Wielder)
	{
		return;
	}

	const FVector SwingStart = Wielder->GetActorLocation();
	const FVector SwingEnd = SwingStart + (Wielder->GetActorForwardVector() * SwingRange);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Wielder);
	ActorsToIgnore.Add(this);

	TArray<FHitResult> Hits;
	UKismetSystemLibrary::SphereTraceMulti(
		this,
		SwingStart,
		SwingEnd,
		SwingRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		Hits,
		true);

	for (const FHitResult& Hit : Hits)
	{
		if (ACharacter* HitCharacter = Cast<ACharacter>(Hit.GetActor()))
		{
			FVector LaunchDirection = HitCharacter->GetActorLocation() - Wielder->GetActorLocation();
			LaunchDirection.Z = 0.f;
			LaunchDirection.Normalize();

			const FVector LaunchVelocity = (LaunchDirection * KnockbackStrength) + (FVector::UpVector * KnockbackUpwardStrength);
			HitCharacter->LaunchCharacter(LaunchVelocity, true, true);
		}
	}
}
