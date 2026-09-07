// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "jinzzaRoundTypes.h"
#include "jinzzaGameGameMode.generated.h"

class AjinzzaPartyPlayerState;

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

	/** Server-only. Transitions Target to ghost status (design doc section 6: mid-evaluation
	 * elimination) via AjinzzaPartyPlayerState::ServerSetGhost, which handles the actual changes
	 * (disguise removed, held prop dropped, movement/emotes still allowed). No-op if Target is
	 * already a ghost or this instance isn't authoritative. This is the entry point a future
	 * mid-evaluation vote-tally system (Week 7, not built yet - see AjinzzaGameGameMode's class
	 * list in the design doc) will call once it can actually name an eliminated candidate; nothing
	 * calls it yet, same "ready once a consumer needs it" pattern as
	 * UjinzzaRoundPhaseSubsystem::NotifyPhaseConditionMet. */
	UFUNCTION(BlueprintCallable, Category = "Round")
	void EliminateToGhost(AjinzzaPartyPlayerState* Target);

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
