// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "jinzzaMenuBackgroundCharacter.generated.h"

class UStaticMeshComponent;

/**
 * A purely decorative, uncontrolled character shown behind the main menu UI - see
 * AjinzzaMenuPlayerController::SetupMenuBackgroundScene, which auto-spawns one (facing
 * AjinzzaMenuCameraRig) if none is hand-placed in Lvl_MainMenu. Never possessed by a controller.
 *
 * Shows the same SM_Seal body the playable character uses (BP_FirstPersonCharacter's SealMesh, a
 * rigid StaticMeshComponent - the seal has no skeleton/animation yet), so the menu backdrop isn't
 * an empty capsule. Since there's no animation, Tick adds a slow "breathing" squash and a gentle
 * side-to-side sway. Lighting comes from AjinzzaMenuSceneDirector, which also moves this
 * character between the menu's backdrop scenes.
 */
UCLASS()
class JINZZA_API AjinzzaMenuBackgroundCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AjinzzaMenuBackgroundCharacter();

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Menu Background")
	TObjectPtr<UStaticMeshComponent> SealMesh;

	/** SealMesh's constructor-time scale/yaw, the base the idle motion in Tick oscillates around. */
	FVector SealBaseScale = FVector(2.f);
	float SealBaseYaw = -90.f;
	float IdleTime = 0.f;
};
