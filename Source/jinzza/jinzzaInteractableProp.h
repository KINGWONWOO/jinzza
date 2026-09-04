// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaInteractableProp.generated.h"

class UStaticMeshComponent;
class USoundBase;
class UTexture2D;
class UWidgetComponent;
class UjinzzaInteractionPromptWidget;

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

	/** Server-only. Detaches a held (Handheld) prop and lets physics settle it where it's dropped. No-op if not currently held. */
	void DropFromHolder();

	/** Server-only. Like DropFromHolder, but launches the prop along LaunchVelocity (cm/s) instead of just letting it fall in place. No-op if not currently held. */
	void ThrowFromHolder(const FVector& LaunchVelocity);

	/** Server-only. Plays this prop's use effects. Called directly for Placed props, or via the holder for Handheld ones. */
	void Activate();

	UFUNCTION(BlueprintPure, Category = "Prop")
	bool IsHeld() const { return HoldingPawn != nullptr; }

	/** True if Holder currently holds this prop (always false for Placed props, or if held by someone else). */
	UFUNCTION(BlueprintPure, Category = "Prop")
	bool IsHeldBy(const APawn* Holder) const { return HoldingPawn == Holder; }

	UFUNCTION(BlueprintPure, Category = "Prop")
	EJinzzaPropInteractionType GetInteractionType() const { return InteractionType; }

	/** For the bottom-right HUD panel explaining how to use whatever's currently held - see UjinzzaPropUsageWidget. */
	UFUNCTION(BlueprintPure, Category = "Prop")
	UTexture2D* GetUsageIcon() const { return UsageIcon; }

	UFUNCTION(BlueprintPure, Category = "Prop")
	FText GetUsageDescription() const { return UsageDescription; }

	/** Client-local cosmetic only - shows/hides the "[F] Pick Up"-style prompt above this prop.
	 * Driven by AjinzzaCharacter::UpdateInteractionFocus tracing what the local player is looking
	 * at; never touches replicated state, so every client can call this independently. */
	void ShowInteractionPrompt();
	void HideInteractionPrompt();

protected:
	/** Server-only. Who's currently holding this prop, if Handheld and picked up - for subclasses like Bat that need to know where to swing from. */
	APawn* GetHoldingPawn() const { return HoldingPawn; }

	/** For subclasses that need to move/animate the mesh directly (e.g. a basketball's dribble bounce). */
	UStaticMeshComponent* GetMesh() const { return Mesh; }

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server-only hook for gameplay-affecting behavior (e.g. Bat's knockback impulse). Runs once, authoritatively. */
	UFUNCTION(BlueprintNativeEvent, Category = "Prop")
	void OnPropActivated();
	virtual void OnPropActivated_Implementation() {}

	/** Runs on every client (including the server) when this prop is used - for per-prop VFX/animation, not gameplay logic. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Prop", meta = (DisplayName = "On Play Use Effects"))
	void BP_OnPlayUseEffects();

	/** Takes the previous holder as a param (engine convention: an OnRep function with one
	 * parameter matching the property's type receives the pre-replication value) so this can
	 * tell whichever locally-controlled character just lost the prop to hide its usage HUD -
	 * see AjinzzaCharacter::ShowPropUsageHUD/HidePropUsageHUD. Manual call sites below
	 * (AttachToHolder/DropFromHolder/ThrowFromHolder) pass the old value explicitly since the
	 * engine only auto-supplies it for real replication, not direct C++ calls. */
	UFUNCTION()
	void OnRep_HoldingPawn(APawn* OldHoldingPawn);

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Screen-space "[F] Pick Up" prompt shown above this prop while the local player looks at
	 * it - see ShowInteractionPrompt/HideInteractionPrompt and UjinzzaInteractionPromptWidget. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWidgetComponent> InteractionPromptComponent;

	/** Widget class for InteractionPromptComponent. Left unset until a Widget Blueprint (e.g.
	 * WBP_InteractionPrompt) exists - the prompt component simply never shows anything until
	 * one is assigned, same "wire content later" pattern as UseSound/ScoreEffect elsewhere. */
	UPROPERTY(EditAnywhere, Category = "Prop")
	TSubclassOf<UjinzzaInteractionPromptWidget> InteractionPromptWidgetClass;

	/** Label shown in the interaction prompt (e.g. "Pick Up" for Handheld, "Use" for Placed). */
	UPROPERTY(EditAnywhere, Category = "Prop")
	FText InteractPromptText = FText::FromString(TEXT("Interact"));

	/** Icon + description for the bottom-right "how to use this" HUD panel while held - see
	 * GetUsageIcon/GetUsageDescription and UjinzzaPropUsageWidget. Left unset by default. */
	UPROPERTY(EditAnywhere, Category = "Prop")
	TObjectPtr<UTexture2D> UsageIcon;

	UPROPERTY(EditAnywhere, Category = "Prop")
	FText UsageDescription;

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
