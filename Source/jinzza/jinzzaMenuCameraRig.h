// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaMenuCameraRig.generated.h"

class UCameraComponent;

/**
 * A placeable camera actor used as the view target for the main menu's 3D backdrop - see
 * AjinzzaMenuPlayerController::SetupMenuBackgroundScene, which auto-spawns one (facing
 * AjinzzaMenuBackgroundCharacter) if none is hand-placed in Lvl_MainMenu. Framing/FOV are tuned per
 * level either on the auto-spawned default or on a hand-placed Blueprint instance in the level.
 */
UCLASS()
class JINZZA_API AjinzzaMenuCameraRig : public AActor
{
	GENERATED_BODY()

public:
	AjinzzaMenuCameraRig();

	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;
};
