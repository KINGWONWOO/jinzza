// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaCharacterPreviewCapture.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaCustomizationApply.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "UObject/ConstructorHelpers.h"

AjinzzaCharacterPreviewCapture::AjinzzaCharacterPreviewCapture()
{
	PrimaryActorTick.bCanEverTick = false;

	PreviewMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewMesh"));
	RootComponent = PreviewMesh;
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// SKM_Seal (a skeletal mesh) no longer exists - the real character's seal body
	// (2026-09-14) is SM_Seal, a rigid StaticMeshComponent with no skeleton/sockets, because the
	// seal has no animation yet. JinzzaCustomization::ApplyToMesh (called from
	// RefreshAppearance()) needs a USkeletalMeshComponent for its Head-socket/hair/accessory
	// attachment logic, so this preview can't be switched to SM_Seal without that customization
	// path being redesigned too - see docs/PROJECT_STATUS.md and ask before doing that redesign.
	// Falling back to the last-known-good Mannequin (same fallback BP_CustomizationTest uses) so
	// CDO construction stops erroring "Failed to find SKM_Seal" on every launch; the preview will
	// look like the Mannequin, not the real seal body, until that redesign happens.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PreviewMeshFinder(TEXT("/Game/JINZZA/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (PreviewMeshFinder.Succeeded())
	{
		PreviewMesh->SetSkeletalMesh(PreviewMeshFinder.Object);
	}

	// Single-node looping Idle so the preview isn't frozen in bind pose - not driven by an
	// AnimBlueprint since this preview never moves/jumps, just idles.
	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimFinder(TEXT("/Game/JINZZA/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	if (IdleAnimFinder.Succeeded())
	{
		PreviewMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		PreviewMesh->AnimationData.AnimToPlay = IdleAnimFinder.Object;
		PreviewMesh->AnimationData.bSavedLooping = true;
		PreviewMesh->AnimationData.bSavedPlaying = true;
	}

	FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(RootComponent);
	FillLight->SetRelativeLocation(FVector(-150.f, -100.f, 200.f));
	FillLight->Intensity = 8000.f;
	FillLight->AttenuationRadius = 800.f;

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(RootComponent);
	Capture->SetRelativeLocation(FVector(200.f, 0.f, 90.f));
	Capture->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	Capture->ProjectionType = ECameraProjectionMode::Perspective;
	Capture->FOVAngle = 30.f;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame = true;
	Capture->bCaptureOnMovement = false;
}

void AjinzzaCharacterPreviewCapture::BeginPlay()
{
	Super::BeginPlay();

	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
	RenderTarget->InitAutoFormat(RenderTargetSize.X, RenderTargetSize.Y);
	RenderTarget->UpdateResourceImmediate(true);

	// bCaptureEveryFrame (set in the constructor) already keeps this updating - an explicit
	// CaptureScene() call here is redundant and logs an "inefficiency" warning.
	Capture->TextureTarget = RenderTarget;

	RefreshAppearance();
}

void AjinzzaCharacterPreviewCapture::RefreshAppearance()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		JinzzaCustomization::ApplyToMesh(PreviewMesh, this, DynamicFaceMaterial, HairMeshComponent,
			DynamicHairMaterial, AccessoryMeshComponent, DynamicAccessoryMaterial, *Settings);
	}
}
