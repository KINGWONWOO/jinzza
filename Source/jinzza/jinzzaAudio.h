// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundAttenuation.h"

class UAudioComponent;
class USoundBase;
class USoundAttenuation;
class UObject;

/**
 * Distance attenuation for every in-world (3D) sound in the game.
 *
 * None of the project's sound assets carry an attenuation asset of their own, so a bare
 * PlaySoundAtLocation plays at full volume no matter how far away the listener is - everyone's
 * footsteps, every prop's use sound and the boombox were equally loud across the whole map. Instead
 * of authoring attenuation assets per sound, every world sound goes through this one profile,
 * described by a single number: the AUDIBLE RADIUS (cm) - the distance beyond which it can't be heard.
 *
 *   distance <= inner radius  : full volume (inner = 15% of the radius, at least 150cm so the
 *                               local player's own footsteps / held prop are never attenuated)
 *   inner .. radius           : "natural sound" falloff down to -40 dB (~1% volume), i.e. volume
 *                               falls off steeply near the source and gently far away
 *   distance >= radius        : silent (the engine also stops mixing it, which saves CPU)
 *
 * Air absorption is on too: farther sounds are progressively low-pass filtered (muffled), from
 * 20 kHz at the inner radius down to 3 kHz at the edge.
 *
 * NOT for UI sounds, menu/lobby music or kiosk confirm beeps - those are PlaySound2D on purpose.
 */
namespace JinzzaAudio
{
	/** How the profile treats a sound that has AudibleRadius <= 0: no attenuation at all, i.e. the old plain behaviour. */
	inline bool IsAttenuated(float AudibleRadius) { return AudibleRadius > 0.f; }

	/** The attenuation profile for AudibleRadius (cm). */
	JINZZA_API FSoundAttenuationSettings MakeAttenuationSettings(float AudibleRadius);

	/** A shared, cached USoundAttenuation asset for AudibleRadius - what PlaySoundAtLocation wants. Never null. */
	JINZZA_API USoundAttenuation* GetAttenuation(float AudibleRadius);

	/**
	 * Plays Sound once at Location, attenuated by AudibleRadius (cm). Overrides any attenuation the sound asset
	 * itself has. AudibleRadius <= 0 plays it unattenuated, exactly like a bare PlaySoundAtLocation.
	 */
	JINZZA_API void PlaySoundAt(const UObject* WorldContext, USoundBase* Sound, const FVector& Location, float AudibleRadius, float VolumeMultiplier = 1.f);

	/** Makes Component (a looping/streaming source, e.g. the boombox) use the profile. Safe to call repeatedly. */
	JINZZA_API void ApplyToComponent(UAudioComponent* Component, float AudibleRadius);

	/**
	 * Same profile for a source that exposes AttenuationOverrides directly (e.g. UMediaSoundComponent): fills Overrides and
	 * returns true if the caller should set its bOverrideAttenuation (false when AudibleRadius <= 0, Overrides untouched).
	 */
	JINZZA_API bool MakeOverrides(FSoundAttenuationSettings& Overrides, float AudibleRadius);

	/**
	 * The volume multiplier (0..1) a listener Distance (cm) away from a source hears under the profile for AudibleRadius -
	 * evaluated with the engine's own attenuation function, so it is exactly what the audio engine applies
	 * (before any per-sound volume). 1 for AudibleRadius <= 0.
	 */
	JINZZA_API float CalcVolumeAtDistance(float AudibleRadius, float Distance);
}
