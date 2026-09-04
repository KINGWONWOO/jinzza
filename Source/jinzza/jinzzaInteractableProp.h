// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaInteractableProp.generated.h"

class UStaticMeshComponent;
class USoundBase;

/**
 * Free-time noise/comedy prop behavior (design doc section 6/11/13): no info-clue props, just
 * stuff that makes FreeTime1/FreeTime2 lively and reactive. Handheld props (Megaphone, Bat,
 * Rubber Duck, ...) get picked up and carried; Placed props (Boombox, Confetti Cannon, ...) are
 * used where they stand. AjinzzaCharacter drives both through a single F-to-interact /
 * left-click-to-use pair (see AjinzzaCharacter::DoInteract/DoUseHeldProp).
 */
UENUM(BlueprintType)
enum class EJinzzaPropInteractionType : uint8
{
	Handheld,
	Placed
};

/**
 * Common base for every free-time prop (design doc's AInteractableProp, section 11). Concrete
 * props (megaphone, bat, boombox, rubber duck, ...) are Blueprint subclasses that just set Mesh
 * asset / UseSound / NoiseRadius / InteractionType and optionally override OnPropActivated for
 * prop-specific behavior (e.g. Bat's knockback) - no new C++ needed per prop.
 *
 * Server-authoritative: AttachToHolder/Activate only ever run on the server (HasAuthority()
 * guards), matching the pattern already established by AjinzzaPartyPlayerState's ServerSet*
 * functions. HoldingPawn is openly replicated (same reasoning as that class - a carried prop is
 * visible to everyone anyway, so hiding it at the network layer buys nothing).
 */
UCLASS(Abstract)
class JINZZA_API AjinzzaInteractableProp : public AActor
{
	GENERATED_BODY()

public:
	AjinzzaInteractableProp();

	/** Server-only. Handheld: attaches to NewHolder's hand socket. Placed: no-op (use Activate instead). */
	void AttachToHolder(APawn* NewHolder);

	/** Server-only. Plays this prop's use effects. Called directly for Placed props, or via the holder for Handheld ones. */
	void Activate();

	UFUNCTION(BlueprintPure, Category = "Prop")
	bool IsHeld() const { return HoldingPawn != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Prop")
	EJinzzaPropInteractionType GetInteractionType() const { return InteractionType; }

protected:
	/** Server-only. Who's currently holding this prop, if Handheld and picked up - for subclasses like Bat that need to know where to swing from. */
	APawn* GetHoldingPawn() const { return HoldingPawn; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server-only hook for gameplay-affecting behavior (e.g. Bat's knockback impulse). Runs once, authoritatively. */
	UFUNCTION(BlueprintNativeEvent, Category = "Prop")
	void OnPropActivated();
	virtual void OnPropActivated_Implementation() {}

	/** Runs on every client (including the server) when this prop is used - for per-prop VFX/animation, not gameplay logic. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Prop", meta = (DisplayName = "On Play Use Effects"))
	void BP_OnPlayUseEffects();

	UFUNCTION()
	void OnRep_HoldingPawn();

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Handheld vs Placed (design doc's two shared interaction montages). */
	UPROPERTY(EditAnywhere, Category = "Prop")
	EJinzzaPropInteractionType InteractionType = EJinzzaPropInteractionType::Handheld;

	/** Played (broadcast to everyone in range) when this prop is activated. */
	UPROPERTY(EditAnywhere, Category = "Prop")
	TObjectPtr<USoundBase> UseSound;

	/** How far the use sound carries - also the doc's "bigger prop use draws more attention" balance knob. */
	UPROPERTY(EditAnywhere, Category = "Prop", meta = (ClampMin = 0, Units = "cm"))
	float NoiseRadius = 1500.f;

	/** Socket on the holder's mesh this prop attaches to when picked up (Handheld only). */
	UPROPERTY(EditAnywhere, Category = "Prop")
	FName HoldSocketName = FName("hand_r");

	UPROPERTY(ReplicatedUsing = OnRep_HoldingPawn)
	TObjectPtr<APawn> HoldingPawn;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayEffects();
	void Multicast_PlayEffects_Implementation();
};
