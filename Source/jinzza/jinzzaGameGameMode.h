// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "jinzzaRoundTypes.h"
#include "jinzzaGameGameMode.generated.h"

/**
 * GameMode for Lvl_Game: drives the round via UjinzzaRoundPhaseSubsystem and assigns roles
 * (AssignRoles()) when the RoleAssignment phase starts.
 */
UCLASS()
class JINZZA_API AjinzzaGameGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AjinzzaGameGameMode();

protected:
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	void OnRoundPhaseEntered(EJinzzaRoundPhase NewPhase);
	void AssignRoles();
	void TryStartRound();

	bool bRoundStarted = false;
	FTimerHandle RoundStartGraceTimerHandle;
};
