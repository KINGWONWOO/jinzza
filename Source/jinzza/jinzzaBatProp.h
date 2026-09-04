// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaInteractableProp.h"
#include "jinzzaBatProp.generated.h"

/**
 * Bat (design doc's 배트, section 6): swinging it knocks back whoever's standing in front of the
 * wielder - pure comedy/reaction prop, no information value ("정보 단서용 소품은 넣지 않는다").
 * Knockback moves another player's character, so unlike most props it needs a small C++ override
 * instead of staying pure Blueprint - see AjinzzaInteractableProp's class comment.
 */
UCLASS()
class JINZZA_API AjinzzaBatProp : public AjinzzaInteractableProp
{
	GENERATED_BODY()

protected:
	virtual void OnPropActivated_Implementation() override;

private:
	/** How far in front of the wielder the swing reaches. */
	UPROPERTY(EditAnywhere, Category = "Bat", meta = (ClampMin = 0, Units = "cm"))
	float SwingRange = 150.f;

	/** Radius of the swing's hit sphere. */
	UPROPERTY(EditAnywhere, Category = "Bat", meta = (ClampMin = 0, Units = "cm"))
	float SwingRadius = 60.f;

	/** Horizontal launch speed applied to whoever gets hit. */
	UPROPERTY(EditAnywhere, Category = "Bat", meta = (ClampMin = 0, Units = "cm/s"))
	float KnockbackStrength = 900.f;

	/** Upward launch speed, so the knockback reads as a comic pop rather than a shove. */
	UPROPERTY(EditAnywhere, Category = "Bat", meta = (ClampMin = 0, Units = "cm/s"))
	float KnockbackUpwardStrength = 300.f;
};
