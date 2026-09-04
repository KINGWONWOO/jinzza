// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "jinzzaRoundTypes.h"
#include "jinzzaGamePlayerController.generated.h"

class UUserWidget;
class APlayerState;

/**
 * Spawns the minimal in-round overlay (host-only End Game button) for Lvl_Game, and receives
 * this player's private role assignment - see AjinzzaGameGameMode::AssignRoles().
 */
UCLASS()
class JINZZA_API AjinzzaGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AjinzzaGamePlayerController();

	/**
	 * Server-only: tells this player their role and, for Imitators, who the Real One is (nullptr
	 * for every other role). Not replicated further - each client only ever learns what the
	 * design doc says they're allowed to know (see AjinzzaPartyPlayerState's class comment).
	 */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveRoleAssignment(EJinzzaPartyRole InRole, APlayerState* InRealOne);

	// Named GetLocalPartyRole (not GetLocalRole) - AActor already declares a GetLocalRole() that
	// returns ENetRole (network role), and UHT rejects a UFUNCTION override with different
	// parameters/return type under that name.
	UFUNCTION(BlueprintPure, Category = "Party")
	EJinzzaPartyRole GetLocalPartyRole() const { return LocalRole; }

	/** Only meaningful when GetLocalPartyRole() == Imitator. */
	UFUNCTION(BlueprintPure, Category = "Party")
	APlayerState* GetKnownRealOne() const { return KnownRealOne; }

	/** Widget class to show. Defaults to UjinzzaGameEndWidget if left unset (WBP_GameEnd if it exists, else the raw C++ class). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> GameEndWidgetClass;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> GameEndWidget;

	EJinzzaPartyRole LocalRole = EJinzzaPartyRole::None;

	UPROPERTY()
	TObjectPtr<APlayerState> KnownRealOne;
};
