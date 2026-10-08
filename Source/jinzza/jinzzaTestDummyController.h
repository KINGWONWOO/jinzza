// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "jinzzaTestDummyController.generated.h"

/**
 * Lvl_test's practice dummies (spawned by AjinzzaTestGameMode): a seal that stands still and looks around.
 * It has its own AjinzzaPartyPlayerState (bWantsPlayerState), so to the match code it's a player like any
 * other - it gets a name tag, a hand-held board, a seat in Question Time, a speaking turn, votes - which
 * is what lets one person try those features alone.
 *
 * Looking around: every few seconds it picks a new direction near the way it's facing and turns toward it.
 * Standing, the whole body turns (a little); seated, only the head does (AjinzzaCharacter::UpdateSeatedLook).
 */
UCLASS()
class JINZZA_API AjinzzaTestDummyController : public AAIController
{
	GENERATED_BODY()

public:
	AjinzzaTestDummyController();

	virtual void Tick(float DeltaSeconds) override;

	/** Where the dummy was spawned - drills put it back here. */
	FTransform HomeTransform = FTransform::Identity;

	/** False for the target dummies by the props (never pulled into a drill). */
	bool bDrillDummy = true;

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	/** The facing it looks around from - its spawn facing, re-read whenever it's teleported (seat, spotlight, home). */
	float RestYaw = 0.f;
	FVector LastPawnLocation = FVector::ZeroVector;

	FRotator LookOffset = FRotator::ZeroRotator;
	FRotator LookTarget = FRotator::ZeroRotator;
	double NextLookTime = 0.0;
};
