// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "jinzzaMatchSettings.h"
#include "jinzzaLobbyTimeOfDay.h"
#include "jinzzaLobbyGameState.generated.h"

/**
 * Replicates the host's chosen match settings to every client in Lvl_Lobby, plus the shared
 * lobby TimeOfDay (Day/Sunset/Night) that AjinzzaLobbyClock cycles - see ApplyTimeOfDayVisuals,
 * which re-times DirectionalLight_0/SkyLight_0 and swings the clock's hands for everyone.
 */
UCLASS()
class JINZZA_API AjinzzaLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated, BlueprintReadOnly)
	FJinzzaMatchSettings MatchSettings;

	UPROPERTY(ReplicatedUsing = OnRep_TimeOfDay, BlueprintReadOnly)
	EJinzzaLobbyTimeOfDay TimeOfDay = EJinzzaLobbyTimeOfDay::Day;

	/** Server-only: advances TimeOfDay by one step (Day -> Sunset -> Night -> Day) and applies it. */
	void CycleTimeOfDay();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_TimeOfDay();

	/**
	 * Re-times DirectionalLight_0/SkyLight_0 to match TimeOfDay and tells AjinzzaLobbyClock to
	 * swing its hands to match. Called directly on the server (OnRep never fires locally there)
	 * and via OnRep_TimeOfDay on every client.
	 */
	void ApplyTimeOfDayVisuals();
};
