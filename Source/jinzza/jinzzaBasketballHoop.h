// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaBasketballHoop.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USoundBase;

/**
 * Placed in a free-time zone: detects AjinzzaBasketballProp passing through its scoring volume,
 * plays ScoreSound, and fires BP_OnPlayScoreEffects for the Blueprint to spawn a Niagara score
 * effect (design doc's Week 10 emotes/props scope, extended per user request). Purely cosmetic
 * feedback - no round-phase/score-tracking hookup exists yet.
 */
UCLASS()
class JINZZA_API AjinzzaBasketballHoop : public AActor
{
	GENERATED_BODY()

public:
	AjinzzaBasketballHoop();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnScoringVolumeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Runs on every client (including whoever scored) when the ball scores - implement in the Hoop Blueprint
	 * to spawn a Niagara score effect (e.g. via "Spawn System at Location"), same pattern as
	 * AjinzzaInteractableProp::BP_OnPlayUseEffects. The effect asset itself is a Blueprint-only variable
	 * (add one in the Blueprint editor) rather than a C++ property, so this module needs no Niagara link
	 * dependency - a UPROPERTY/UFUNCTION referencing a UNiagaraSystem type still requires linking Niagara
	 * even without calling into it directly, since UHT-generated reflection code calls its exported
	 * class-construction function. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Hoop", meta = (DisplayName = "On Play Score Effects"))
	void BP_OnPlayScoreEffects();

private:
	UPROPERTY(VisibleAnywhere, Category = "Hoop")
	TObjectPtr<UStaticMeshComponent> HoopMesh;

	/** Small trigger volume just under the rim - the ball scores when it overlaps this falling through. */
	UPROPERTY(VisibleAnywhere, Category = "Hoop")
	TObjectPtr<USphereComponent> ScoringVolume;

	UPROPERTY(EditAnywhere, Category = "Hoop")
	TObjectPtr<USoundBase> ScoreSound;

	/** Cooldown (seconds) after a score before the same ball can score again, so one pass through the volume as it settles doesn't trigger multiple times. */
	UPROPERTY(EditAnywhere, Category = "Hoop", meta = (ClampMin = 0, Units = "s"))
	float ScoreCooldown = 1.f;

	/** Runs on every client (including whoever scored), same pattern as the props' use-effects. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayScoreEffects();

	UFUNCTION()
	void ClearScoreCooldown();

	bool bOnCooldown = false;
	FTimerHandle CooldownTimerHandle;
};
