// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaAuditionCurtain.generated.h"

class UStaticMeshComponent;

/**
 * Self-introduction zone stage curtain: rises to reveal the spotlight stage, drops to hide it.
 * Purely cosmetic and replicated - opened/closed server-side by AjinzzaGameGameMode on
 * SelfIntroduction phase enter/exit (see AjinzzaGameGameMode::UpdateZoneForPhase), not by direct
 * player interaction. Every instance (server + each client) interpolates independently toward the
 * replicated bOpen target in Tick, so there's no need for exact multi-client timing.
 */
UCLASS()
class JINZZA_API AjinzzaAuditionCurtain : public AActor
{
	GENERATED_BODY()

public:
	AjinzzaAuditionCurtain();

	/** Server-only. Raises the curtain (no-op if already open/opening). */
	UFUNCTION(BlueprintCallable, Category = "Curtain")
	void Open();

	/** Server-only. Lowers the curtain (no-op if already closed/closing). */
	UFUNCTION(BlueprintCallable, Category = "Curtain")
	void Close();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Curtain")
	TObjectPtr<UStaticMeshComponent> CurtainMesh;

	/** How far above its closed position the curtain rises when open, in cm. */
	UPROPERTY(EditAnywhere, Category = "Curtain")
	float RaiseHeight = 400.f;

	/** Seconds for a full raise/lower transition. */
	UPROPERTY(EditAnywhere, Category = "Curtain")
	float TransitionSeconds = 2.f;

	UPROPERTY(Replicated)
	bool bOpen = false;

	/** CurtainMesh's authored relative Z at BeginPlay - treated as the "closed" position, whatever
	 * height the mesh was actually placed at in the level. */
	float ClosedRelativeZ = 0.f;
};
