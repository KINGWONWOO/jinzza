// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaMenuSceneDirector.generated.h"

class UStaticMesh;
class AjinzzaMenuBackgroundCharacter;
class AjinzzaMenuCameraRig;
class APlayerController;

/** One static-mesh set piece in a menu scene. Transform is relative to the scene origin (where the seal stands, facing +X). */
USTRUCT(BlueprintType)
struct FJinzzaMenuScenePiece
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Scene")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, Category = "Scene")
	FTransform Transform;
};

/**
 * One camera move across a scene's whole shot. All points are relative to the scene origin. The
 * camera always looks at the (interpolated) focus point. With bOrbit, the camera swings around the
 * focus (angle, distance and height interpolated separately) instead of travelling in a straight
 * line from Start to End.
 */
USTRUCT(BlueprintType)
struct FJinzzaMenuCameraMove
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector Start = FVector(450.f, 0.f, 130.f);

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector End = FVector(450.f, 0.f, 130.f);

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector StartFocus = FVector(0.f, 0.f, 110.f);

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector EndFocus = FVector(0.f, 0.f, 110.f);

	UPROPERTY(EditAnywhere, Category = "Camera")
	float StartFOV = 50.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float EndFOV = 50.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	bool bOrbit = false;
};

/** One main-menu backdrop scene: a lit "stage" with a few props, a mood color, and its own camera move. */
USTRUCT(BlueprintType)
struct FJinzzaMenuScene
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Scene")
	FName Name;

	/** Mood color: drives the colored rim light and lightly tints the overhead lamp. */
	UPROPERTY(EditAnywhere, Category = "Scene")
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Scene")
	FLinearColor FloorColor = FLinearColor(0.02f, 0.02f, 0.025f);

	UPROPERTY(EditAnywhere, Category = "Scene")
	TArray<FJinzzaMenuScenePiece> Props;

	UPROPERTY(EditAnywhere, Category = "Scene")
	FJinzzaMenuCameraMove Camera;
};

/**
 * Drives the main menu's 3D backdrop as a loop of short "cuts": builds every scene in Scenes once
 * (side by side, SceneSpacing apart along +X from this actor, far from Lvl_MainMenu's own geometry)
 * with its own dark floor, props and lights - an overhead spotlight lamp straight above the seal,
 * a soft front fill, and a colored rim light. Every ShotDuration seconds it fades to black, moves
 * the AjinzzaMenuBackgroundCharacter into the next scene and starts that scene's camera move on
 * AjinzzaMenuCameraRig, then fades back in.
 *
 * The darkness comes from switching off the level's DirectionalLight/SkyLight while the menu runs
 * (runtime only - Lvl_MainMenu itself isn't changed) plus a fixed exposure and strong vignette on
 * the menu camera, so auto-exposure can't brighten the black surroundings back up.
 *
 * Spawned by AjinzzaMenuPlayerController::SetupMenuBackgroundScene if none is hand-placed, and
 * started from there (Start) once the character and camera rig exist. The scene list defaults
 * are built in the constructor and are EditAnywhere, so a hand-placed instance can be re-dressed.
 */
UCLASS()
class JINZZA_API AjinzzaMenuSceneDirector : public AActor
{
	GENERATED_BODY()

public:
	AjinzzaMenuSceneDirector();

	virtual void Tick(float DeltaSeconds) override;

	/** Builds the scenes, darkens the level and starts the loop at scene 0. Safe to call once. */
	void Start(AjinzzaMenuBackgroundCharacter* InCharacter, AjinzzaMenuCameraRig* InCameraRig, APlayerController* InPlayerController);

	UPROPERTY(EditAnywhere, Category = "Menu Scenes")
	TArray<FJinzzaMenuScene> Scenes;

	/** How long each cut stays on screen before the next one, in seconds. */
	UPROPERTY(EditAnywhere, Category = "Menu Scenes", meta = (ClampMin = "2.0"))
	float ShotDuration = 10.f;

	/** Fade-to-black time at each end of a cut, in seconds. */
	UPROPERTY(EditAnywhere, Category = "Menu Scenes", meta = (ClampMin = "0.0"))
	float FadeDuration = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Menu Scenes")
	float SceneSpacing = 5000.f;

	/** Fixed exposure (EV100) for the menu camera, tuned against the light intensities in BuildScene. */
	UPROPERTY(EditAnywhere, Category = "Menu Scenes")
	float ExposureEV100 = 3.f;

private:
	void BuildScene(int32 SceneIndex);
	void DarkenLevelLights();
	void ApplyCameraPostProcess();
	void EnterScene(int32 SceneIndex);
	void UpdateCamera(float Alpha);
	FVector GetSceneOrigin(int32 SceneIndex) const;

	UPROPERTY(Transient)
	TObjectPtr<AjinzzaMenuBackgroundCharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<AjinzzaMenuCameraRig> CameraRig;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> PlayerController;

	int32 CurrentScene = INDEX_NONE;
	float ShotElapsed = 0.f;
	bool bFadingOut = false;
};
