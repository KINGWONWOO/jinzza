// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "jinzzaCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UjinzzaDisguiseComponent;
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

	/** Pick up / activate a placed prop Input Action (F) */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* InteractAction;

	/** Use the currently held prop Input Action (Left Mouse Button) */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* UseHeldPropAction;

	/** How far (cm) the F-key trace reaches when looking for a prop to interact with. */
	UPROPERTY(EditAnywhere, Category ="Input", meta = (ClampMin = 0, Units = "cm"))
	float InteractTraceDistance = 300.f;

	/** Server-only. The prop currently attached to this character, if any - never needs to be known by clients (they just ask the server to use it). */
	TObjectPtr<AjinzzaInteractableProp> HeldProp;

public:
	AjinzzaCharacter();

protected:

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

	/** Traces from the camera for a prop and requests the server pick it up (Handheld) or activate it in place (Placed) */
	void DoInteract();

	/** Requests the server activate whatever prop this character is currently holding */
	void DoUseHeldProp();

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	/** Server-only. Attaches Prop (Handheld) or activates it immediately (Placed). */
	UFUNCTION(Server, Reliable)
	void Server_InteractWithProp(AjinzzaInteractableProp* Prop);

	/** Server-only. Activates HeldProp, if any. */
	UFUNCTION(Server, Reliable)
	void Server_UseHeldProp();

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/** Returns the prop currently attached to this character, if any (e.g. for a future voice component to query a held Megaphone). */
	UFUNCTION(BlueprintPure, Category = "Prop")
	AjinzzaInteractableProp* GetHeldProp() const { return HeldProp; }

};

