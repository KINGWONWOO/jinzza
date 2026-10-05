// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Components/ActorComponent.h"
#include "jinzzaInteractHighlight.generated.h"

class APostProcessVolume;

/**
 * White outline on what the local player can interact with: the focused prop, and every lobby
 * kiosk/clock (always on, set once in AjinzzaLobbyPlayerController::BeginPlay). The outline itself is the post-process material
 * /Game/JINZZA/Materials/PostProcess/M_PP_InteractOutline: it draws a 3px white edge around every
 * mesh rendering CustomDepth, only where that mesh isn't hidden behind something closer.
 * SetHighlighted just toggles RenderCustomDepth on an actor's meshes; the focus code calls it
 * (AjinzzaCharacter::UpdateInteractionFocus via the prop's Show/HideInteractionPrompt,
 * AjinzzaLobbyPlayerController::CheckForNearbyKiosk). Client-local and cosmetic only.
 */
namespace JinzzaHighlight
{
	JINZZA_API void SetHighlighted(AActor* Actor, bool bHighlighted);
}

/**
 * Adds M_PP_InteractOutline to every game world (lobby, match, test levels) by spawning an
 * unbound post-process volume that only carries that blendable - so no level needs it set up by
 * hand. Skipped on dedicated servers (nothing renders there).
 */
UCLASS()
class JINZZA_API UjinzzaInteractHighlightSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UPROPERTY()
	TObjectPtr<APostProcessVolume> OutlineVolume;
};

/**
 * Keeps the owner's text labels turned toward the local player's camera (yaw only), so a kiosk
 * or clock label is always readable from wherever you stand. Rotates every UTextRenderComponent
 * and world-space UWidgetComponent on the owning actor: their readable side is +X, so +X is
 * pointed at the camera. Ticks on its own, so it works even on actors with actor ticking off
 * (every kiosk). Added automatically by AjinzzaInteractableKiosk; can also be added to any other
 * actor with labels.
 */
UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class JINZZA_API UjinzzaFaceCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UjinzzaFaceCameraComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
