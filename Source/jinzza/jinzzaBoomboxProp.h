// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaInteractableProp.h"
#include "jinzzaBoomboxProp.generated.h"

class UAudioComponent;
class USoundBase;

/**
 * Boombox (design doc's 라디오/스피커, section 6/13: Placed type): pressing interact toggles a
 * looping background track on/off in place.
 *
 * v1 plays a single fixed Track rather than an arbitrary URL/playlist - streaming an arbitrary
 * URL means a MediaPlayer/WebMediaSource kept in sync across every client, real extra work
 * compared to every other prop. A curated Track (or later, a small TArray<USoundBase*>
 * playlist cycled by repeated presses) is the cheap version, and slots into the same
 * OnPropActivated/OnRep_IsPlaying toggle below without changing it.
 */
UCLASS()
class JINZZA_API AjinzzaBoomboxProp : public AjinzzaInteractableProp
{
	GENERATED_BODY()

public:
	AjinzzaBoomboxProp();

	UFUNCTION(BlueprintPure, Category = "Boombox")
	bool IsPlaying() const { return bIsPlaying; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnPropActivated_Implementation() override;

	UFUNCTION()
	void OnRep_IsPlaying();

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UAudioComponent> AudioComponent;

	/** Looping background track played while on. */
	UPROPERTY(EditAnywhere, Category = "Boombox")
	TObjectPtr<USoundBase> Track;

	UPROPERTY(ReplicatedUsing = OnRep_IsPlaying)
	bool bIsPlaying = false;
};
