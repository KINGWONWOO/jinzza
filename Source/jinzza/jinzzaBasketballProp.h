// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaInteractableProp.h"
#include "jinzzaBasketballProp.generated.h"

/**
 * Basketball (free-time prop, design doc section 6/11/13 scope extended per user request):
 * dribbles a cosmetic bounce in place each time it's used (left click, base class's existing
 * F-to-pick-up / left-click-to-use pair) while held, and can be thrown
 * (AjinzzaInteractableProp::ThrowFromHolder, wired to Right Mouse Button on AjinzzaCharacter) -
 * see AjinzzaBasketballHoop for scoring.
 */
UCLASS()
class JINZZA_API AjinzzaBasketballProp : public AjinzzaInteractableProp
{
	GENERATED_BODY()

protected:
	virtual void OnPropActivated_Implementation() override;

private:
	/** How far down (cm) the cosmetic dribble bounce dips before returning. */
	UPROPERTY(EditAnywhere, Category = "Basketball", meta = (ClampMin = 0, Units = "cm"))
	float BounceHeight = 15.f;

	/** How long (seconds) one dribble bounce takes, down and back up. */
	UPROPERTY(EditAnywhere, Category = "Basketball", meta = (ClampMin = 0.01, Units = "s"))
	float BounceDuration = 0.25f;

	/** Runs the bounce on every client (including whoever's dribbling), same as the base class's use-effects. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayBounce();

	void TickBounce();

	FTimerHandle BounceTimerHandle;
	float BounceElapsed = 0.f;
	FVector MeshRestRelativeLocation = FVector::ZeroVector;
};
