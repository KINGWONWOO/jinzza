// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/VoiceConfig.h"
#include "jinzzaRoundTypes.h"
#include "jinzzaProximityVoiceComponent.generated.h"

class UAudioComponent;
class USoundEffectSourcePresetChain;
class AjinzzaCharacter;

/**
 * Proximity voice chat for one character (design doc section 8-2 / 11, "UProximityVoiceComponent").
 *
 * Built on the engine's own VOIP (OnlineSubsystem voice - Steam in a packaged build, the Null subsystem in
 * PIE; enabled by [Voice] bEnabled / [OnlineSubsystem] bHasVoiceEnabled in DefaultEngine.ini) rather than
 * EOS/Vivox, which needs dev-portal credentials this project doesn't have yet. It's a UVOIPTalker: once
 * registered with the owning player's PlayerState, the voice engine plays that player's incoming voice
 * through the settings below on every OTHER player's machine:
 *
 *   - Positional: attached to AjinzzaCharacter::VoiceAnchor (mouth height), attenuated by the same
 *     JinzzaAudio profile as every other world sound, audible out to VoiceRadius.
 *   - Megaphone: while the speaker holds an amplifying AjinzzaMegaphoneProp, the radius grows to the
 *     megaphone's GetAmplifiedVoiceRadius().
 *   - Disguise filter: AjinzzaPartyPlayerState::VoiceFilter (High/Low/Robot), or Robot while the speaker is
 *     stunned (AjinzzaCharacter::IsStunned - see AjinzzaStunGunProp). There's no real-time pitch shifter in
 *     the engine's Synthesis plugin, so High/Low are timbre approximations (filters + chorus), not true pitch.
 *   - Walls: a static-geometry trace from the local listener to the speaker; each wall in between muffles
 *     (low-pass) and quiets the voice, and more than MaxWallsHeard walls silences it - so Lvl_Game's sealed
 *     rooms (incl. the 1:1 interview room) don't leak voice into each other.
 *   - Ghosts: a ghost's voice is silenced for everyone (design doc: "유령 상태 마이크 강제 비활성화");
 *     AjinzzaPlayerController also stops the ghost's own mic.
 *
 * Settings (radius, filter) are picked up by the voice engine when the speaker STARTS talking; volume and
 * wall muffling are applied live every WallCheckInterval to the audio component the engine hands us.
 * Who transmits, and when (push-to-talk vs open mic), is AjinzzaPlayerController's job.
 */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent))
class JINZZA_API UjinzzaProximityVoiceComponent : public UVOIPTalker
{
	GENERATED_BODY()

public:
	UjinzzaProximityVoiceComponent(const FObjectInitializer& ObjectInitializer);

	/** The point the voice plays from. Set by AjinzzaCharacter in BeginPlay. */
	void SetVoiceAnchor(USceneComponent* InAnchor);

	/** True while this player's voice is currently playing on this machine. */
	UFUNCTION(BlueprintPure, Category = "Voice")
	bool IsTalking() const { return ActiveVoiceComponent.IsValid(); }

	/** The disguise filter this player's voice currently plays through (stun overrides the round disguise). */
	UFUNCTION(BlueprintPure, Category = "Voice")
	EJinzzaVoiceFilter GetEffectiveVoiceFilter() const;

	/** How far this player's voice carries right now (cm) - bigger while amplified by a megaphone. */
	UFUNCTION(BlueprintPure, Category = "Voice")
	float GetEffectiveVoiceRadius() const;

	virtual void OnTalkingBegin(UAudioComponent* AudioComponent) override;
	virtual void OnTalkingEnd() override;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Normal speaking voice range (cm). */
	UPROPERTY(EditAnywhere, Category = "Voice", meta = (ClampMin = 0, Units = "cm"))
	float VoiceRadius = 2500.f;

	/** Volume multiplier per wall between listener and speaker. */
	UPROPERTY(EditAnywhere, Category = "Voice|Walls", meta = (ClampMin = 0, ClampMax = 1))
	float VolumePerWall = 0.25f;

	/** Low-pass cutoff (Hz) once at least one wall is in the way - the "through the wall" muffle. */
	UPROPERTY(EditAnywhere, Category = "Voice|Walls", meta = (ClampMin = 100, Units = "Hz"))
	float WallLowPassFrequency = 900.f;

	/** More walls than this and the voice isn't heard at all (1 = a voice carries through one wall, muffled). */
	UPROPERTY(EditAnywhere, Category = "Voice|Walls", meta = (ClampMin = 0))
	int32 MaxWallsHeard = 1;

	/** How often registration, live volume and walls are re-evaluated (s). */
	UPROPERTY(EditAnywhere, Category = "Voice", meta = (ClampMin = 0.02, Units = "s"))
	float UpdateInterval = 0.1f;

	/** Registers with the owner's PlayerState once it has replicated (and again if something else took the slot). */
	void UpdateRegistration();

	/** Pushes radius/filter into UVOIPTalker::Settings, read by the voice engine on the next talk start. */
	void UpdateVoiceSettings();

	/** Live volume/muffle on the playing voice (walls, ghost, player's voice volume setting). */
	void UpdateActiveVoice();

	/** Number of static walls between this machine's listener and the voice anchor (capped at MaxWallsHeard + 1). */
	int32 CountWallsToListener() const;

	USoundEffectSourcePresetChain* GetFilterChain(EJinzzaVoiceFilter Filter);

	AjinzzaCharacter* GetCharacter() const;

	TWeakObjectPtr<USceneComponent> VoiceAnchor;
	TWeakObjectPtr<UAudioComponent> ActiveVoiceComponent;
	TWeakObjectPtr<APlayerState> RegisteredPlayerState;

	/** Radius the playing voice was last given (see UpdateVoiceSettings). */
	float ActiveVoiceRadius = 0.f;

	/** One effect chain per filter, built on first use. */
	UPROPERTY(Transient)
	TMap<EJinzzaVoiceFilter, TObjectPtr<USoundEffectSourcePresetChain>> FilterChains;
};
