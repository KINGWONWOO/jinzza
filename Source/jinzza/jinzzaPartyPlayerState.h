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

	/**
	 * Name for nameplates and chat. In the match every candidate (Real One and Imitators alike, in random
	 * order) is "User1".."UserN" and the Judge is "Judge", so names never give a role away - see
	 * AjinzzaGameGameMode::AssignDisplayAliases. Outside the match (no alias) it's the player's nickname.
	 */
	UFUNCTION(BlueprintPure, Category = "Party")
	FString GetDisplayName() const { return DisplayAlias.IsEmpty() ? GetPlayerName() : DisplayAlias; }

	bool HasDisplayAlias() const { return !DisplayAlias.IsEmpty(); }

	/** N for a "UserN" candidate alias, 0 otherwise (no alias yet, or the Judge). Used for speaking order. */
	int32 GetAliasUserNumber() const;

	/** Has a candidate alias and isn't a ghost - i.e. can take a speaking turn / be voted for. */
	bool IsLivingCandidate() const { return GetAliasUserNumber() > 0 && !IsGhost(); }

	/** Server-only. Not carried over by seamless travel (CopyProperties), so it's gone again back in the lobby. */
	void ServerSetDisplayAlias(const FString& NewAlias);

	/** GetDisplayName for any PlayerState (plain APlayerState in levels that don't use this class). */
	static FString GetDisplayNameFor(const APlayerState* PlayerState);

	UFUNCTION(BlueprintPure, Category = "Party")
	bool IsGhost() const { return bIsGhost; }
	/** Server-only. Transitioning to true (design doc section 6: mid-evaluation elimination) also
	 * clears FaceType/VoiceFilter ("위장 해제" - disguise fully removed, not just visually ignored)
	 * and force-drops whatever prop the owning AjinzzaCharacter was holding, since a ghost can no
	 * longer pick up or use props (see AjinzzaCharacter::IsGhost). Movement/emotes stay available -
	 * "탈락은 처벌이 아니라 전환" (elimination is a transition, not a punishment). The ghost's voice is
	 * force-muted (design doc section 8-2) by AjinzzaPlayerController (stops transmitting) and
	 * UjinzzaProximityVoiceComponent (silenced on every listener), both reading IsGhost(). */
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

	UPROPERTY(Replicated)
	FString DisplayAlias;

	UPROPERTY(ReplicatedUsing = OnRep_DisguiseChanged)
	EJinzzaFaceType FaceType = EJinzzaFaceType::None;

	UPROPERTY(ReplicatedUsing = OnRep_DisguiseChanged)
	EJinzzaVoiceFilter VoiceFilter = EJinzzaVoiceFilter::None;
};
