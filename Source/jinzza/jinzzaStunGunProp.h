// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaInteractableProp.h"
#include "jinzzaStunGunProp.generated.h"

/**
 * Stun gun (electric shock device, free-time prop): using it zaps whoever's in front of the
 * wielder, immobilizing them for a few seconds (AjinzzaCharacter::Stun) - same
 * sphere-trace-in-front-of-wielder hit detection as AjinzzaBatProp, just immobilizing instead
 * of launching. Placeholder art only (temporary model, per user request) - see
 * BP_StunGun's Mesh component.
 *
 * While stunned, the target's voice plays robotized for everyone (UjinzzaProximityVoiceComponent
 * reads AjinzzaCharacter::IsStunned and overrides the round disguise filter for the duration -
 * AjinzzaPartyPlayerState::VoiceFilter itself is left untouched).
 *
 * NOT YET IMPLEMENTED - deferred, recorded here so it isn't forgotten:
 *   - Any stun VFX/SFX (screen shake, electric zap sound, a "stunned" icon over the victim's
 *     head) - AjinzzaCharacter::IsStunned() is real and replicated, so this is just a matter of
 *     a Blueprint/widget reading it once someone wants to add the presentation.
 */
UCLASS()
class JINZZA_API AjinzzaStunGunProp : public AjinzzaInteractableProp
{
	GENERATED_BODY()

public:
	/** Sets HoldRepeatInterval so holding the use button keeps shocking (and keeps the stun refreshed) until released. */
	AjinzzaStunGunProp();

protected:
	virtual void OnPropActivated_Implementation() override;

private:
	/** How far in front of the wielder the zap reaches. */
	UPROPERTY(EditAnywhere, Category = "StunGun", meta = (ClampMin = 0, Units = "cm"))
	float Range = 200.f;

	/** Radius of the zap's hit sphere. */
	UPROPERTY(EditAnywhere, Category = "StunGun", meta = (ClampMin = 0, Units = "cm"))
	float Radius = 60.f;

	/** How long a hit target is immobilized for. */
	UPROPERTY(EditAnywhere, Category = "StunGun", meta = (ClampMin = 0, Units = "s"))
	float StunDuration = 3.f;
};
