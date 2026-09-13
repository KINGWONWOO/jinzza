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

	// Same mesh the real playable character (BP_FirstPersonCharacter) uses, so the preview
	// actually matches what a live pawn looks like. Was SKM_Manny_Simple (the Mannequin) until
	// the real character was reskinned to the seal - this preview is a separate hardcoded
	// ConstructorHelpers reference (not read from the live character class), so it had to be
	// updated here too or it would silently keep showing the old Mannequin forever.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PreviewMeshFinder(TEXT("/Game/JINZZA/Characters/seal/SKM_Seal.SKM_Seal"));
	if (PreviewMeshFinder.Succeeded())
	{
		PreviewMesh->SetSkeletalMesh(PreviewMeshFinder.Object);
	}

	// Single-node looping Idle so the preview isn't frozen in bind pose - same stopgap
	// AjinzzaCharacter's real Mesh component uses until a proper AnimBP exists (see
	// jinzzaCharacter.cpp's history) - not driven by an AnimBlueprint since this preview never
	// moves/jumps, just idles.
	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimFinder(TEXT("/Game/JINZZA/Characters/seal/SKM_Seal_Anim_Armature_Idle.SKM_Seal_Anim_Armature_Idle"));
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
