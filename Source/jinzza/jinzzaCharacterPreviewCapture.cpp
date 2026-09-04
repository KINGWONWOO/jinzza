// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaCharacterPreviewCapture.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"

AjinzzaCharacterPreviewCapture::AjinzzaCharacterPreviewCapture()
{
	PrimaryActorTick.bCanEverTick = false;

	PreviewMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewMesh"));
	RootComponent = PreviewMesh;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannequinFinder(TEXT("/Game/JINZZA/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"));
	if (MannequinFinder.Succeeded())
	{
		PreviewMesh->SetSkeletalMesh(MannequinFinder.Object);
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
}
