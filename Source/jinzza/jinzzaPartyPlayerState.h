// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "jinzzaRoundTypes.h"
#include "jinzzaPartyPlayerState.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnJinzzaDisguiseChanged);

/**
 * Per-player round state for Lvl_Game. Ghost status and face/voice disguise are openly
 * replicated - they're visually/aurally observable in-game by everyone anyway, so hiding them
 * at the network layer would gain nothing. What must stay hidden is the *role* itself: the
 * design doc's core principle is "정보 비대칭이 곧 서스펜스다" (information asymmetry is the
 * suspense) - the Judge must not be able to infer who's who from replicated data. So ServerRole
 * is deliberately NOT a UPROPERTY and never replicates; it only ever exists on the server, and
 * each client learns only what they're allowed to know via a targeted Client RPC sent from
 * AjinzzaGameGameMode::AssignRoles() (see AjinzzaGamePlayerController::Client_ReceiveRoleAssignment).
 */
UCLASS()
class JINZZA_API AjinzzaPartyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/** Server-only. Never replicated - see class comment. */
	EJinzzaPartyRole ServerRole = EJinzzaPartyRole::None;

	UFUNCTION(BlueprintPure, Category = "Party")
	bool IsGhost() const { return bIsGhost; }
	/** Server-only. */
	void ServerSetGhost(bool bNewGhost);

	UFUNCTION(BlueprintPure, Category = "Party")
	EJinzzaFaceType GetFaceType() const { return FaceType; }
	/** Server-only. */
	void ServerSetFaceType(EJinzzaFaceType NewFaceType);

	UFUNCTION(BlueprintPure, Category = "Party")
	EJinzzaVoiceFilter GetVoiceFilter() const { return VoiceFilter; }
	/** Server-only. */
	void ServerSetVoiceFilter(EJinzzaVoiceFilter NewVoiceFilter);

	/**
	 * Broadcast on both server and clients whenever ghost/face/voice changes, so
	 * UjinzzaDisguiseComponent can re-apply visuals. Also pokes the owning Pawn's disguise
	 * component directly (see .cpp) so the common case - role assignment after the pawn already
	 * exists - doesn't rely on anyone remembering to bind this delegate.
	 */
	FOnJinzzaDisguiseChanged OnDisguiseChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_DisguiseChanged();

	UPROPERTY(ReplicatedUsing = OnRep_DisguiseChanged)
	bool bIsGhost = false;

	UPROPERTY(ReplicatedUsing = OnRep_DisguiseChanged)
	EJinzzaFaceType FaceType = EJinzzaFaceType::None;

	UPROPERTY(ReplicatedUsing = OnRep_DisguiseChanged)
	EJinzzaVoiceFilter VoiceFilter = EJinzzaVoiceFilter::None;
};
