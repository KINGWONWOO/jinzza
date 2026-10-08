// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaGamePlayerController.h"
#include "jinzzaTestGameMode.h"
#include "jinzzaTestPlayerController.generated.h"

class AjinzzaInteractableKiosk;
class UjinzzaTestHudWidget;

/**
 * Lvl_test's player controller (AjinzzaTestGameMode): the match controller - so the match HUD (speaking turns,
 * Question Time, the ballot) works for the drills - plus walking up to kiosks like in the lobby (every
 * kiosk in the level, the lobby ones included) with an on-screen prompt. Plays like the rest of the test
 * level: no cursor except while a panel / the drawing screen / the ballot needs one; no End Game button or
 * match music.
 */
UCLASS()
class JINZZA_API AjinzzaTestPlayerController : public AjinzzaGamePlayerController
{
	GENERATED_BODY()

public:
	/** AjinzzaTestDrillKiosk -> AjinzzaTestGameMode::StartDrill. */
	UFUNCTION(Server, Reliable)
	void Server_StartDrill(EJinzzaTestDrill Drill);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual bool UsesMatchExtras() const override { return false; }
	/** A kiosk panel (or the drawing screen) has the cursor - no ESC menu on top of it. */
	virtual bool CanOpenPauseMenu() const override { return !bShowMouseCursor; }

private:
	void CheckForNearbyKiosk();
	void OnInteractPressed();

	UPROPERTY(Transient)
	TObjectPtr<UjinzzaTestHudWidget> TestHud;

	UPROPERTY(Transient)
	TObjectPtr<AjinzzaInteractableKiosk> NearbyKiosk;

	FTimerHandle KioskCheckTimerHandle;

	/** Kiosks already outlined (drill stations are spawned at start-up, possibly after this controller). */
	TSet<TWeakObjectPtr<AjinzzaInteractableKiosk>> HighlightedKiosks;
};
