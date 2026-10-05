// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMenuSceneDirector.h"
#include "jinzzaMenuBackgroundCharacter.h"
#include "jinzzaMenuCameraRig.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Components/LightComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"

namespace
{
	FJinzzaMenuScenePiece MakeMenuScenePiece(const TCHAR* MeshPath, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator, const FVector& Scale = FVector::OneVector)
	{
		FJinzzaMenuScenePiece Piece;
		Piece.Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath));
		Piece.Transform = FTransform(Rotation, Location, Scale);
		return Piece;
	}

	FJinzzaMenuCameraMove MakeMenuCameraMove(const FVector& Start, const FVector& End, const FVector& StartFocus, const FVector& EndFocus,
		float StartFOV, float EndFOV, bool bOrbit = false)
	{
		FJinzzaMenuCameraMove Move;
		Move.Start = Start;
		Move.End = End;
		Move.StartFocus = StartFocus;
		Move.EndFocus = EndFocus;
		Move.StartFOV = StartFOV;
		Move.EndFOV = EndFOV;
		Move.bOrbit = bOrbit;
		return Move;
	}

	USpotLightComponent* AddMenuSpotLight(AActor* Owner, const FVector& Location, const FVector& AimAt, const FLinearColor& Color,
		float Candelas, float InnerCone, float OuterCone)
	{
		USpotLightComponent* Light = NewObject<USpotLightComponent>(Owner);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetupAttachment(Owner->GetRootComponent());
		Light->RegisterComponent();
		Light->SetWorldLocationAndRotation(Location, (AimAt - Location).Rotation());
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(Candelas);
		Light->SetLightColor(Color);
		Light->SetInnerConeAngle(InnerCone);
		Light->SetOuterConeAngle(OuterCone);
		Light->SetAttenuationRadius(1500.f);
		return Light;
	}

	UStaticMeshComponent* AddMenuStaticMesh(AActor* Owner, UStaticMesh* Mesh, const FTransform& WorldTransform)
	{
		UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(Owner);
		MeshComponent->SetMobility(EComponentMobility::Movable);
		MeshComponent->SetStaticMesh(Mesh);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetupAttachment(Owner->GetRootComponent());
		MeshComponent->RegisterComponent();
		MeshComponent->SetWorldTransform(WorldTransform);
		return MeshComponent;
	}
}

AjinzzaMenuSceneDirector::AjinzzaMenuSceneDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	// Five default scenes. Scene frame: the seal stands at the origin facing +X, so the camera
	// works in the +X half. Layout/lighting/framing was previewed in the editor with temporary
	// actors (2026-10-04) - see docs/todo.txt.
	const TCHAR* ChamferCube = TEXT("/Game/JINZZA/LevelPrototyping/Meshes/SM_ChamferCube.SM_ChamferCube");
	const TCHAR* Cylinder = TEXT("/Game/JINZZA/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder");

	// 1. Interrogation room - cold white lamp, the seal behind a desk with "evidence" on it. Slow push-in.
	{
		FJinzzaMenuScene& Scene = Scenes.AddDefaulted_GetRef();
		Scene.Name = TEXT("Interrogation");
		Scene.LightColor = FLinearColor(0.55f, 0.70f, 1.f);
		Scene.FloorColor = FLinearColor(0.020f, 0.020f, 0.025f);
		Scene.Props.Add(MakeMenuScenePiece(ChamferCube, FVector(115.f, 0.f, 36.f), FRotator::ZeroRotator, FVector(0.8f, 1.9f, 0.72f)));
		Scene.Props.Add(MakeMenuScenePiece(TEXT("/Game/JINZZA/Props/Meshes/StunGun/SM_StunGun.SM_StunGun"), FVector(110.f, 45.f, 75.f), FRotator(90.f, 30.f, 0.f), FVector(1.5f)));
		Scene.Props.Add(MakeMenuScenePiece(ChamferCube, FVector(-40.f, 0.f, 45.f), FRotator::ZeroRotator, FVector(0.9f, 1.1f, 0.9f)));
		Scene.Camera = MakeMenuCameraMove(FVector(560.f, 0.f, 175.f), FVector(410.f, 0.f, 150.f), FVector(0.f, 0.f, 115.f), FVector(0.f, 0.f, 125.f), 50.f, 46.f);
	}

	// 2. Stage - magenta lamp, stand mic and boombox. Orbits from the seal's left to its right.
	{
		FJinzzaMenuScene& Scene = Scenes.AddDefaulted_GetRef();
		Scene.Name = TEXT("Stage");
		Scene.LightColor = FLinearColor(1.f, 0.30f, 0.70f);
		Scene.FloorColor = FLinearColor(0.025f, 0.015f, 0.025f);
		Scene.Props.Add(MakeMenuScenePiece(TEXT("/Game/JINZZA/Props/Meshes/StandMic/SM_StandMic.SM_StandMic"), FVector(110.f, -70.f, 80.f)));
		Scene.Props.Add(MakeMenuScenePiece(TEXT("/Game/JINZZA/Props/Meshes/Boombox/SM_Boombox.SM_Boombox"), FVector(30.f, 135.f, 37.f), FRotator(0.f, -60.f, 0.f), FVector(1.5f)));
		Scene.Camera = MakeMenuCameraMove(FVector(345.f, -289.f, 130.f), FVector(345.f, 289.f, 130.f), FVector(0.f, 0.f, 100.f), FVector(0.f, 0.f, 100.f), 48.f, 48.f, true);
	}

	// 3. Court - orange lamp, hoop behind the seal, ball on the floor. Cranes up from low to high.
	{
		FJinzzaMenuScene& Scene = Scenes.AddDefaulted_GetRef();
		Scene.Name = TEXT("Court");
		Scene.LightColor = FLinearColor(1.f, 0.55f, 0.20f);
		Scene.FloorColor = FLinearColor(0.030f, 0.020f, 0.012f);
		Scene.Props.Add(MakeMenuScenePiece(Cylinder, FVector(-150.f, 0.f, 0.f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 2.9f)));
		Scene.Props.Add(MakeMenuScenePiece(TEXT("/Game/JINZZA/Props/Meshes/Backboard/SM_Backboard.SM_Backboard"), FVector(-140.f, 0.f, 300.f), FRotator(0.f, 90.f, 0.f), FVector(1.6f)));
		Scene.Props.Add(MakeMenuScenePiece(TEXT("/Game/JINZZA/Props/Meshes/HoopRim/SM_HoopRim.SM_HoopRim"), FVector(-95.f, 0.f, 255.f), FRotator(0.f, 90.f, 0.f), FVector(1.6f)));
		Scene.Props.Add(MakeMenuScenePiece(TEXT("/Game/JINZZA/Props/Meshes/Basketball/SM_Basketball.SM_Basketball"), FVector(90.f, -85.f, 18.f), FRotator::ZeroRotator, FVector(1.5f)));
		Scene.Camera = MakeMenuCameraMove(FVector(360.f, 80.f, 45.f), FVector(430.f, -60.f, 290.f), FVector(0.f, 0.f, 120.f), FVector(0.f, 0.f, 110.f), 55.f, 55.f);
	}

	// 4. Back alley - crimson "suspect" lamp, stacked crates and a bat. Trucks sideways past the seal.
	{
		FJinzzaMenuScene& Scene = Scenes.AddDefaulted_GetRef();
		Scene.Name = TEXT("Alley");
		Scene.LightColor = FLinearColor(1.f, 0.10f, 0.08f);
		Scene.FloorColor = FLinearColor(0.025f, 0.012f, 0.012f);
		Scene.Props.Add(MakeMenuScenePiece(ChamferCube, FVector(-130.f, -140.f, 50.f), FRotator(0.f, 15.f, 0.f)));
		Scene.Props.Add(MakeMenuScenePiece(ChamferCube, FVector(-140.f, -130.f, 135.f), FRotator(0.f, -10.f, 0.f), FVector(0.7f)));
		Scene.Props.Add(MakeMenuScenePiece(TEXT("/Game/JINZZA/Props/Meshes/Bat/SM_Bat.SM_Bat"), FVector(-95.f, 120.f, 48.f), FRotator(0.f, 0.f, -18.f)));
		Scene.Camera = MakeMenuCameraMove(FVector(460.f, -260.f, 120.f), FVector(460.f, 260.f, 130.f), FVector(0.f, -30.f, 105.f), FVector(0.f, 30.f, 105.f), 46.f, 46.f);
	}

	// 5. Announcement - gold lamp, megaphone on a podium. Pulls out from a close-up of the face.
	{
		FJinzzaMenuScene& Scene = Scenes.AddDefaulted_GetRef();
		Scene.Name = TEXT("Announcement");
		Scene.LightColor = FLinearColor(1.f, 0.75f, 0.30f);
		Scene.FloorColor = FLinearColor(0.025f, 0.022f, 0.015f);
		Scene.Props.Add(MakeMenuScenePiece(Cylinder, FVector(85.f, -120.f, 0.f), FRotator::ZeroRotator, FVector(0.6f, 0.6f, 0.9f)));
		Scene.Props.Add(MakeMenuScenePiece(TEXT("/Game/JINZZA/Props/Meshes/Megaphone/SM_Megaphone.SM_Megaphone"), FVector(85.f, -120.f, 120.f), FRotator::ZeroRotator, FVector(1.3f)));
		Scene.Camera = MakeMenuCameraMove(FVector(260.f, 0.f, 165.f), FVector(620.f, 0.f, 175.f), FVector(0.f, 0.f, 150.f), FVector(0.f, 0.f, 100.f), 38.f, 50.f);
	}
}

void AjinzzaMenuSceneDirector::Start(AjinzzaMenuBackgroundCharacter* InCharacter, AjinzzaMenuCameraRig* InCameraRig, APlayerController* InPlayerController)
{
	if (CurrentScene != INDEX_NONE || Scenes.Num() == 0)
	{
		return;
	}

	Character = InCharacter;
	CameraRig = InCameraRig;
	PlayerController = InPlayerController;

	for (int32 SceneIndex = 0; SceneIndex < Scenes.Num(); ++SceneIndex)
	{
		BuildScene(SceneIndex);
	}

	DarkenLevelLights();
	ApplyCameraPostProcess();
	EnterScene(0);
	SetActorTickEnabled(true);
}

FVector AjinzzaMenuSceneDirector::GetSceneOrigin(int32 SceneIndex) const
{
	return GetActorLocation() + FVector(SceneIndex * SceneSpacing, 0.f, 0.f);
}

void AjinzzaMenuSceneDirector::BuildScene(int32 SceneIndex)
{
	const FJinzzaMenuScene& Scene = Scenes[SceneIndex];
	const FVector Origin = GetSceneOrigin(SceneIndex);

	// Dark floor, big enough that its edges fall outside the lamp's pool into darkness.
	if (UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
	{
		UStaticMeshComponent* Floor = AddMenuStaticMesh(this, PlaneMesh, FTransform(FRotator::ZeroRotator, Origin, FVector(30.f, 30.f, 1.f)));
		if (UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			UMaterialInstanceDynamic* FloorMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			FloorMaterial->SetVectorParameterValue(TEXT("Color"), Scene.FloorColor);
			Floor->SetMaterial(0, FloorMaterial);
		}
	}

	for (const FJinzzaMenuScenePiece& Piece : Scene.Props)
	{
		if (UStaticMesh* Mesh = Piece.Mesh.LoadSynchronous())
		{
			FTransform WorldTransform = Piece.Transform;
			WorldTransform.AddToTranslation(Origin);
			AddMenuStaticMesh(this, Mesh, WorldTransform);
		}
	}

	// Lighting, aimed at the seal's middle (~110cm up). The overhead lamp is only lightly tinted
	// so the seal keeps its own colors - a fully colored lamp turned it solid magenta/red in the
	// editor preview; the mood color is carried by the rim light and the floor instead.
	const FVector SealCenter = Origin + FVector(0.f, 0.f, 110.f);
	const FLinearColor LampColor = FMath::Lerp(FLinearColor::White, Scene.LightColor, 0.3f);
	AddMenuSpotLight(this, Origin + FVector(0.f, 0.f, 480.f), Origin, LampColor, 250.f, 14.f, 26.f);
	AddMenuSpotLight(this, Origin + FVector(320.f, -60.f, 200.f), SealCenter, FLinearColor(1.f, 0.95f, 0.9f), 25.f, 25.f, 40.f);
	AddMenuSpotLight(this, Origin + FVector(-220.f, 150.f, 260.f), SealCenter, Scene.LightColor, 220.f, 20.f, 35.f);
}

void AjinzzaMenuSceneDirector::DarkenLevelLights()
{
	// Runtime only - leaves Lvl_MainMenu's saved lights alone. Lvl_MainMenu is only ever the menu,
	// so nothing needs restoring when it unloads.
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		if (ULightComponent* Light = It->GetLightComponent())
		{
			Light->SetVisibility(false);
		}
	}
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		if (USkyLightComponent* Light = It->GetLightComponent())
		{
			Light->SetVisibility(false);
		}
	}
}

void AjinzzaMenuSceneDirector::ApplyCameraPostProcess()
{
	UCameraComponent* Camera = CameraRig ? CameraRig->GetCameraComponent() : nullptr;
	if (!Camera)
	{
		return;
	}

	// Fixed exposure: with auto-exposure the mostly-black frame would get brightened until the
	// "darkness" around the lamp turned grey again.
	FPostProcessSettings& Settings = Camera->PostProcessSettings;
	Settings.bOverride_AutoExposureMinBrightness = true;
	Settings.AutoExposureMinBrightness = ExposureEV100;
	Settings.bOverride_AutoExposureMaxBrightness = true;
	Settings.AutoExposureMaxBrightness = ExposureEV100;
	Settings.bOverride_VignetteIntensity = true;
	Settings.VignetteIntensity = 0.9f;
	Camera->PostProcessBlendWeight = 1.f;
}

void AjinzzaMenuSceneDirector::EnterScene(int32 SceneIndex)
{
	CurrentScene = SceneIndex;
	ShotElapsed = 0.f;
	bFadingOut = false;

	if (Character)
	{
		const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Character->SetActorLocationAndRotation(GetSceneOrigin(SceneIndex) + FVector(0.f, 0.f, HalfHeight), FRotator::ZeroRotator,
			false, nullptr, ETeleportType::TeleportPhysics);
	}

	UpdateCamera(0.f);

	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		PlayerController->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeDuration, FLinearColor::Black, false, false);
	}
}

void AjinzzaMenuSceneDirector::UpdateCamera(float Alpha)
{
	if (!CameraRig || !Scenes.IsValidIndex(CurrentScene))
	{
		return;
	}

	const FJinzzaMenuCameraMove& Move = Scenes[CurrentScene].Camera;
	const FVector Origin = GetSceneOrigin(CurrentScene);
	const float Eased = FMath::InterpSinInOut(0.f, 1.f, Alpha);

	const FVector Focus = Origin + FMath::Lerp(Move.StartFocus, Move.EndFocus, Eased);
	FVector CameraLocation;
	if (Move.bOrbit)
	{
		// Swing around the focus point: angle, horizontal distance and height interpolate on their own.
		const FVector StartArm = Move.Start - Move.StartFocus;
		const FVector EndArm = Move.End - Move.EndFocus;
		const float StartAngle = FMath::RadiansToDegrees(FMath::Atan2(StartArm.Y, StartArm.X));
		const float EndAngle = FMath::RadiansToDegrees(FMath::Atan2(EndArm.Y, EndArm.X));
		const float Angle = FMath::DegreesToRadians(StartAngle + FMath::FindDeltaAngleDegrees(StartAngle, EndAngle) * Eased);
		const float Distance = FMath::Lerp(StartArm.Size2D(), EndArm.Size2D(), Eased);
		const float Height = FMath::Lerp(StartArm.Z, EndArm.Z, Eased);
		CameraLocation = Focus + FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, Height);
	}
	else
	{
		CameraLocation = Origin + FMath::Lerp(Move.Start, Move.End, Eased);
	}

	CameraRig->SetActorLocationAndRotation(CameraLocation, (Focus - CameraLocation).Rotation());
	if (UCameraComponent* Camera = CameraRig->GetCameraComponent())
	{
		Camera->SetFieldOfView(FMath::Lerp(Move.StartFOV, Move.EndFOV, Eased));
	}
}

void AjinzzaMenuSceneDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Scenes.IsValidIndex(CurrentScene))
	{
		return;
	}

	ShotElapsed += DeltaSeconds;

	if (ShotElapsed >= ShotDuration)
	{
		EnterScene((CurrentScene + 1) % Scenes.Num());
		return;
	}

	if (!bFadingOut && ShotElapsed >= ShotDuration - FadeDuration)
	{
		bFadingOut = true;
		if (PlayerController && PlayerController->PlayerCameraManager)
		{
			// Hold black until EnterScene fades the next cut back in.
			PlayerController->PlayerCameraManager->StartCameraFade(0.f, 1.f, FadeDuration, FLinearColor::Black, false, true);
		}
	}

	UpdateCamera(ShotElapsed / ShotDuration);
}
