// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMenuBackgroundCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AjinzzaMenuBackgroundCharacter::AjinzzaMenuBackgroundCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;

	// Same body/scale as BP_FirstPersonCharacter's SealMesh, sitting on the bottom of the capsule.
	// SM_Seal's own forward is its local +Y, hence the -90 yaw so it faces along the actor's +X
	// (toward the menu camera - see AjinzzaMenuPlayerController::SetupMenuBackgroundScene).
	SealMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SealMesh"));
	SealMesh->SetupAttachment(GetCapsuleComponent());
	SealMesh->SetRelativeLocation(FVector(0.f, 0.f, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
	SealMesh->SetRelativeRotation(FRotator(0.f, SealBaseYaw, 0.f));
	SealMesh->SetRelativeScale3D(SealBaseScale);
	SealMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SealMeshFinder(TEXT("/Game/JINZZA/Characters/seal/SM_Seal.SM_Seal"));
	if (SealMeshFinder.Succeeded())
	{
		SealMesh->SetStaticMesh(SealMeshFinder.Object);
	}
}

void AjinzzaMenuBackgroundCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!SealMesh)
	{
		return;
	}

	IdleTime += DeltaSeconds;

	// Breathing: a slight vertical stretch with matching horizontal squash (volume-ish preserving),
	// ~0.4 Hz. Sway: a slow +-6 degree look left/right at a different rate so the two don't sync up.
	const float Breath = FMath::Sin(IdleTime * 2.5f) * 0.02f;
	SealMesh->SetRelativeScale3D(SealBaseScale * FVector(1.f - Breath * 0.5f, 1.f - Breath * 0.5f, 1.f + Breath));
	SealMesh->SetRelativeRotation(FRotator(0.f, SealBaseYaw + FMath::Sin(IdleTime * 0.6f) * 6.f, 0.f));
}
