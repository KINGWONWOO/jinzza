// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaBasketballProp.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

void AjinzzaBasketballProp::OnPropActivated_Implementation()
{
	Multicast_PlayBounce();
}

void AjinzzaBasketballProp::Multicast_PlayBounce_Implementation()
{
	UStaticMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	if (!GetWorldTimerManager().IsTimerActive(BounceTimerHandle))
	{
		MeshRestRelativeLocation = MeshComp->GetRelativeLocation();
	}

	BounceElapsed = 0.f;
	GetWorldTimerManager().SetTimer(BounceTimerHandle, this, &AjinzzaBasketballProp::TickBounce, 0.016f, true);
}

void AjinzzaBasketballProp::TickBounce()
{
	UStaticMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		GetWorldTimerManager().ClearTimer(BounceTimerHandle);
		return;
	}

	BounceElapsed += 0.016f;
	const float Alpha = FMath::Clamp(BounceElapsed / BounceDuration, 0.f, 1.f);
	// A single downward arc: 0 at start/end, -BounceHeight at the midpoint.
	const float Offset = -BounceHeight * FMath::Sin(Alpha * PI);
	MeshComp->SetRelativeLocation(MeshRestRelativeLocation + FVector(0.f, 0.f, Offset));

	if (Alpha >= 1.f)
	{
		MeshComp->SetRelativeLocation(MeshRestRelativeLocation);
		GetWorldTimerManager().ClearTimer(BounceTimerHandle);
	}
}
