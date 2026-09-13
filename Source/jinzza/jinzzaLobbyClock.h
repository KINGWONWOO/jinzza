// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaInteractableKiosk.h"
#include "jinzzaLobbyTimeOfDay.h"
#include "jinzzaLobbyClock.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Wall clock prop in Lvl_Lobby. Interacting (host-only, mirrors AjinzzaStartMatchKiosk's
 * HasAuthority() check) advances the shared TimeOfDay on AjinzzaLobbyGameState one step
 * (Day -> Sunset -> Night -> Day), which re-times DirectionalLight_0/SkyLight_0 for everyone -
 * see AjinzzaLobbyGameState::ApplyTimeOfDayVisuals - and swings this clock's own hands to match
 * via SetDisplayedTime, called from there.
 *
 * Built from composite Engine basic-shape meshes (a flattened Cylinder face plus two Cube
 * hands), same placeholder-art pattern as every other prop in this project - see
 * [[placeholder-prop-meshes]].
 */
UCLASS()
class JINZZA_API AjinzzaLobbyClock : public AjinzzaInteractableKiosk
{
	GENERATED_BODY()

public:
	AjinzzaLobbyClock();

	virtual FText GetInteractionPrompt() const override { return FText::FromString(TEXT("Press E - Change Time of Day")); }

	/** Host-only: advances the shared lobby TimeOfDay by one step. */
	virtual void Interact(APlayerController* Interactor) override;

	/** Swings HourHand to the face position associated with NewTimeOfDay (minute hand stays at 12). */
	void SetDisplayedTime(EJinzzaLobbyTimeOfDay NewTimeOfDay);

private:
	UPROPERTY(VisibleAnywhere, Category = "Clock")
	TObjectPtr<USceneComponent> ClockRoot;

	UPROPERTY(VisibleAnywhere, Category = "Clock")
	TObjectPtr<UStaticMeshComponent> Face;

	// Hands are meshes offset from - and parented to - a separate zero-length pivot at the
	// clock's center, not rotated directly themselves: a component's own RelativeRotation spins
	// its mesh in place around its own (fixed) RelativeLocation, it does NOT also revolve that
	// location around the parent - so rotating the mesh component directly produced a
	// centered "+" instead of a swinging hand. Rotating the PIVOT's Roll instead correctly
	// sweeps the (fixed-offset) child mesh around the pivot's origin. Roll (not Yaw) is the
	// sweep axis because Face's own Pitch=90 puts the clock face in the actor's local Y-Z plane.
	UPROPERTY(VisibleAnywhere, Category = "Clock")
	TObjectPtr<USceneComponent> HourPivot;

	UPROPERTY(VisibleAnywhere, Category = "Clock")
	TObjectPtr<UStaticMeshComponent> HourHand;

	UPROPERTY(VisibleAnywhere, Category = "Clock")
	TObjectPtr<USceneComponent> MinutePivot;

	UPROPERTY(VisibleAnywhere, Category = "Clock")
	TObjectPtr<UStaticMeshComponent> MinuteHand;

	UPROPERTY(VisibleAnywhere, Category = "Clock")
	TObjectPtr<UTextRenderComponent> Label;
};
