// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaTestDummyController.h"
#include "jinzzaCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** How far (degrees) a dummy glances left/right: a little while standing (the body turns), more seated (just the head). */
	constexpr float DummyStandingLookYaw = 30.f;
	constexpr float DummySeatedLookYaw = 70.f;
	constexpr float DummyLookPitch = 12.f;
	/** Moved further than this in one tick = teleported. */
	constexpr float DummyTeleportDistance = 50.f;
}

AjinzzaTestDummyController::AjinzzaTestDummyController()
{
	PrimaryActorTick.bCanEverTick = true;
	// A player state of the game mode's class (AjinzzaPartyPlayerState): name, alias, role, ghost, revealed drawing.
	bWantsPlayerState = true;
	// The look-around below owns the control rotation.
	bSetControlRotationFromPawnOrientation = false;
}

void AjinzzaTestDummyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn)
	{
		RestYaw = InPawn->GetActorRotation().Yaw;
		LastPawnLocation = InPawn->GetActorLocation();
		SetControlRotation(FRotator(0.f, RestYaw, 0.f));
	}
}

void AjinzzaTestDummyController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APawn* MyPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!MyPawn || !World)
	{
		return;
	}

	const AjinzzaCharacter* DummyCharacter = Cast<AjinzzaCharacter>(MyPawn);
	const bool bSeated = DummyCharacter && DummyCharacter->IsSeated();

	// Teleported somewhere new: look around from its new facing, starting straight ahead.
	if (FVector::DistSquared(MyPawn->GetActorLocation(), LastPawnLocation) > FMath::Square(DummyTeleportDistance))
	{
		RestYaw = MyPawn->GetActorRotation().Yaw;
		LookOffset = LookTarget = FRotator::ZeroRotator;
	}
	LastPawnLocation = MyPawn->GetActorLocation();

	const double Now = World->GetTimeSeconds();
	if (Now >= NextLookTime)
	{
		const float Range = bSeated ? DummySeatedLookYaw : DummyStandingLookYaw;
		// Now and then just look straight ahead again.
		LookTarget = FMath::FRand() < 0.3f
			? FRotator::ZeroRotator
			: FRotator(FMath::FRandRange(-DummyLookPitch, DummyLookPitch), FMath::FRandRange(-Range, Range), 0.f);
		NextLookTime = Now + FMath::FRandRange(1.5f, 3.5f);
	}

	LookOffset = FMath::RInterpTo(LookOffset, LookTarget, DeltaSeconds, 3.f);
	const float BaseYaw = bSeated ? DummyCharacter->GetSeatYaw() : RestYaw;
	SetControlRotation(FRotator(LookOffset.Pitch, BaseYaw + LookOffset.Yaw, 0.f));
	if (!bSeated)
	{
		MyPawn->FaceRotation(GetControlRotation(), DeltaSeconds);
	}
}
