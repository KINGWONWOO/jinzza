// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "jinzzaEmoteTypes.h"
#include "jinzzaCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UAnimMontage;
class USoundBase;
class UjinzzaDisguiseComponent;
class UjinzzaCharacterCustomizationComponent;
class UjinzzaEmoteWheelWidget;
class UjinzzaPropUsageWidget;
class AjinzzaInteractableProp;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class AjinzzaCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Applies this player's round disguise (face material) to the third-person mesh other players see. No-op outside Lvl_Game (AjinzzaPartyPlayerState won't be the active PlayerState class there). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UjinzzaDisguiseComponent> DisguiseComponent;

	/** Applies the local player's saved appearance (head/hair/top/eyebrows/eyes) - see UjinzzaCharacterCustomizationComponent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UjinzzaCharacterCustomizationComponent> CustomizationComponent;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;

	/** Sprint Input Action (held) */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* SprintInputAction;

	/** Multiplier applied to the character's walk speed (captured at BeginPlay) while sprinting. */
	UPROPERTY(EditAnywhere, Category ="Input", meta = (ClampMin = "1.0"))
	float SprintSpeedMultiplier = 1.6f;

	/** Pick up / activate a placed prop Input Action (F) */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* InteractAction;

	/** Use the currently held prop Input Action (Left Mouse Button) */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* UseHeldPropAction;

	/** Drop the currently held prop Input Action (Q) */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* DropHeldPropAction;

	/** Open the radial emote menu Input Action (E, held) */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* EmoteWheelAction;

	/** Throw the currently held prop Input Action (Right Mouse Button) */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ThrowHeldPropAction;

	/** Launch speed (cm/s) applied to a thrown prop, along wherever the camera is currently aiming. */
	UPROPERTY(EditAnywhere, Category ="Input", meta = (ClampMin = 0, Units = "cm/s"))
	float ThrowSpeed = 1500.f;

	/** How far (cm) the F-key trace reaches when looking for a prop to interact with. */
	UPROPERTY(EditAnywhere, Category ="Input", meta = (ClampMin = 0, Units = "cm"))
	float InteractTraceDistance = 300.f;

	/** Footstep sound played at a roughly even stride interval while walking/sprinting on the
	 * ground - alternates with FootstepSoundAlt for a little variety. TEMP placeholder audio
	 * (see [[placeholder-prop-meshes]]-style project convention) - defaults to the project's
	 * generic Footstep asset until real per-surface sounds exist. */
	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<USoundBase> FootstepSound;

	/** Second footstep variant, randomly alternated with FootstepSound. */
	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<USoundBase> FootstepSoundAlt;

	/** Ground distance (cm) the character must cover between footstep sounds - roughly one stride. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (ClampMin = "1.0", Units = "cm"))
	float FootstepDistanceInterval = 150.f;

	/** Played once when this character actually leaves the ground under their own input (Jump()). */
	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<USoundBase> JumpSound;

	/** Played once when this character lands back on the ground after being airborne. */
	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<USoundBase> LandSound;

	/** Ground distance covered since the last footstep sound - see UpdateFootsteps. */
	float DistanceSinceLastFootstep = 0.f;

	/** Server-only. The prop currently attached to this character, if any - never needs to be known by clients (they just ask the server to use it). */
	TObjectPtr<AjinzzaInteractableProp> HeldProp;

	/** Widget class for the radial emote menu. Defaults to WBP_EmoteWheel if it exists, else the raw C++ class. */
	UPROPERTY(EditAnywhere, Category = "Emote")
	TSubclassOf<UjinzzaEmoteWheelWidget> EmoteWheelWidgetClass;

	/** Montages for each emote-wheel direction. Leave unassigned to no-op that emote harmlessly until animation content exists. */
	UPROPERTY(EditAnywhere, Category = "Emote")
	TObjectPtr<UAnimMontage> ThumbsUpMontage;

	UPROPERTY(EditAnywhere, Category = "Emote")
	TObjectPtr<UAnimMontage> ThumbsDownMontage;

	UPROPERTY(EditAnywhere, Category = "Emote")
	TObjectPtr<UAnimMontage> MiddleFingerMontage;

	UPROPERTY(EditAnywhere, Category = "Emote")
	TObjectPtr<UAnimMontage> PointMontage;

	/** TEMP placeholder sound effects for each emote-wheel direction - swap for real SFX later.
	 * Unlike the montages above these already have defaults (set in the constructor), so emotes
	 * are audible today even with no animation content yet. */
	UPROPERTY(EditAnywhere, Category = "Emote")
	TObjectPtr<USoundBase> ThumbsUpSound;

	UPROPERTY(EditAnywhere, Category = "Emote")
	TObjectPtr<USoundBase> ThumbsDownSound;

	UPROPERTY(EditAnywhere, Category = "Emote")
	TObjectPtr<USoundBase> MiddleFingerSound;

	UPROPERTY(EditAnywhere, Category = "Emote")
	TObjectPtr<USoundBase> PointSound;

	/** True while the radial emote menu is open - suppresses camera look so mouse movement steers the wheel instead. */
	bool bEmoteWheelOpen = false;

	UPROPERTY()
	TObjectPtr<UjinzzaEmoteWheelWidget> EmoteWheelWidget;

	/** Bottom-right HUD panel explaining how to use whatever's currently held. Defaults to
	 * WBP_PropUsageHUD if it exists, else the raw C++ class. */
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UjinzzaPropUsageWidget> PropUsageWidgetClass;

	UPROPERTY()
	TObjectPtr<UjinzzaPropUsageWidget> PropUsageWidget;

	/** Which prop's info the HUD is currently showing, if any - lets HidePropUsageHUD ignore a
	 * stale call for a prop that isn't the one on screen anymore. */
	TWeakObjectPtr<AjinzzaInteractableProp> HUDDisplayedProp;

	/** Whichever interactable prop the local player is currently looking at, if any - see
	 * UpdateInteractionFocus. Never set on remote proxies (only the locally-controlled
	 * character traces for this). */
	TWeakObjectPtr<AjinzzaInteractableProp> FocusedInteractProp;

public:
	AjinzzaCharacter();

	/** Client-local: shows this character's prop usage HUD for Prop. Called by
	 * AjinzzaInteractableProp::OnRep_HoldingPawn when Prop starts being held by this character
	 * specifically - harmless to call on a non-locally-controlled character (PropUsageWidget is
	 * only ever created for the local player in BeginPlay, so it's just null there). */
	void ShowPropUsageHUD(AjinzzaInteractableProp* Prop);

	/** Client-local: hides the prop usage HUD, but only if it's currently showing Prop (guards
	 * against a stale Hide arriving after a different prop has already replaced it on screen). */
	void HidePropUsageHUD(AjinzzaInteractableProp* Prop);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Handles sprint start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintStart();

	/** Handles sprint end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintEnd();

	/** Traces from the camera for a prop and requests the server pick it up (Handheld) or activate it in place (Placed) */
	void DoInteract();

	/** Shared trace logic for both DoInteract (on F press) and UpdateInteractionFocus (every tick, for the prompt). */
	AjinzzaInteractableProp* TraceForInteractableProp() const;

	/** Every tick, shows/hides the interaction prompt on whichever prop the local player is currently looking at. */
	void UpdateInteractionFocus();

	/** Plays a footstep sound at a roughly even stride interval while moving on the ground. Runs
	 * on every instance (not just the locally-controlled one) since replicated movement drives a
	 * simulated proxy's velocity too - so nearby players hear each other's footsteps from their
	 * own machine's Tick without needing an RPC. Purely cosmetic, never gameplay-authoritative. */
	void UpdateFootsteps(float DeltaSeconds);

	/** Plays LandSound when this character touches back down after being airborne. */
	virtual void Landed(const FHitResult& Hit) override;

	/** Requests the server activate whatever prop this character is currently holding */
	void DoUseHeldProp();

	/** Requests the server drop whatever prop this character is currently holding */
	void DoDropHeldProp();

	/** Requests the server throw whatever prop this character is currently holding, launched along the current aim direction */
	void DoThrowHeldProp();

	/** Opens the radial emote menu locally and frees the mouse cursor to steer it (E pressed) */
	void DoOpenEmoteWheel();

	/** Closes the radial emote menu, restores normal camera look, and requests the server play whichever emote was hovered (E released) */
	void DoCloseEmoteWheel();

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server-only. Attaches Prop (Handheld) or activates it immediately (Placed). */
	UFUNCTION(Server, Reliable)
	void Server_InteractWithProp(AjinzzaInteractableProp* Prop);

	/** Server-only. Activates HeldProp, if any. */
	UFUNCTION(Server, Reliable)
	void Server_UseHeldProp();

	/** Server-only. Drops HeldProp, if any, and clears it. */
	UFUNCTION(Server, Reliable)
	void Server_DropHeldProp();

	/** Server-only. Throws HeldProp, if any, along the current aim direction, and clears it. */
	UFUNCTION(Server, Reliable)
	void Server_ThrowHeldProp();

	/** Server-only. Broadcasts EmoteType to every client via Multicast_PlayEmote. */
	UFUNCTION(Server, Reliable)
	void Server_PlayEmote(EJinzzaEmoteType EmoteType);

	/** Tells the server to authoritatively set bIsSprinting (and this character's MaxWalkSpeed), which then replicates to every client. Called from DoSprintStart/DoSprintEnd on non-authority machines, after they've already applied the change locally for immediate feedback. */
	UFUNCTION(Server, Reliable)
	void Server_SetSprinting(bool bNewSprinting);

	/** Plays EmoteType's montage on this character's mesh for every client, including whoever triggered it. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayEmote(EJinzzaEmoteType EmoteType);

private:
	UAnimMontage* GetMontageForEmote(EJinzzaEmoteType EmoteType) const;
	USoundBase* GetSoundForEmote(EJinzzaEmoteType EmoteType) const;

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/** Returns the prop currently attached to this character, if any (e.g. for a future voice component to query a held Megaphone). */
	UFUNCTION(BlueprintPure, Category = "Prop")
	AjinzzaInteractableProp* GetHeldProp() const { return HeldProp; }

	/** Server-only. Clears HeldProp if it currently points at Prop - called by AjinzzaInteractableProp::AttachToHolder when this character's held prop is snatched away by someone else pressing F on it. */
	void ClearHeldPropIfMatches(const AjinzzaInteractableProp* Prop);

	/** Server-only. Drops HeldProp, if any - same effect as Server_DropHeldProp, but callable
	 * directly server-side (no RPC round-trip needed) since this is also used to force a prop out
	 * of a player's hands the instant they become a ghost (AjinzzaPartyPlayerState::ServerSetGhost),
	 * not just in response to their own Q-press. */
	void ServerForceDropHeldProp();

	/** True once this character's AjinzzaPartyPlayerState says they've been eliminated to ghost
	 * status (design doc section 6: mid-evaluation elimination). Movement/look/jump/sprint/emotes
	 * stay available - only prop interaction is blocked, see DoInteract/DoUseHeldProp/etc. Always
	 * false outside Lvl_Game or before a PlayerState exists yet. */
	UFUNCTION(BlueprintPure, Category = "Party")
	bool IsGhost() const;

	/** True while immobilized (e.g. hit by AjinzzaStunGunProp) - movement/jump input is ignored until the stun timer clears it. Replicated so it reaches the owning client's own input handlers too. */
	UFUNCTION(BlueprintPure, Category = "Prop")
	bool IsStunned() const { return bStunned; }

	/** Server-only. Immobilizes this character for Duration seconds (movement/jump input ignored - see DoMove/DoJumpStart). Called by props like AjinzzaStunGunProp.
	 *
	 * NOT YET IMPLEMENTED (deferred - see AjinzzaStunGunProp's class comment): making the
	 * stunned character's voice sound mechanical. That needs the proximity voice system
	 * (Vivox/EOS, design doc section 11, Week 6), which doesn't exist yet - same gap
	 * EJinzzaVoiceFilter::Robot already has everywhere else in the project. */
	void Stun(float Duration);

private:
	UFUNCTION()
	void OnRep_Stunned();

	UFUNCTION()
	void ClearStun();

	/** Applies bNewSprinting to bIsSprinting and MaxWalkSpeed immediately (for local prediction on
	 * whichever machine calls it - server or client), then, if this machine isn't authoritative,
	 * asks the server to do the same via Server_SetSprinting so it becomes the replicated truth. */
	void SetSprinting(bool bNewSprinting);

	/** Applies bIsSprinting's replicated value to MaxWalkSpeed - fires on every client (including
	 * the one that predicted it) whenever the server's copy of bIsSprinting changes. */
	UFUNCTION()
	void OnRep_IsSprinting();

	/** Server-only. */
	UPROPERTY(ReplicatedUsing = OnRep_Stunned)
	bool bStunned = false;

	FTimerHandle StunTimerHandle;

	/** GetCharacterMovement()->MaxWalkSpeed as configured on this character (captured once in
	 * BeginPlay), so DoSprintStart/DoSprintEnd can scale relative to it and restore it exactly,
	 * whatever value the Blueprint has it set to. */
	float BaseWalkSpeed = 0.f;

	/** True while sprinting. Replicated so remote clients see the correct speed too, not just
	 * whichever machine pressed the key - see SetSprinting/Server_SetSprinting. */
	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting)
	bool bIsSprinting = false;

};

