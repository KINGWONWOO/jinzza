// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaInteractableProp.h"
#include "jinzzaMegaphoneProp.generated.h"

/**
 * Megaphone (design doc's 확성기, section 6): while active, amplifies the wielder's voice to a
 * much larger radius. Unlike most props (one-shot use), this one toggles on/off each time it's
 * used.
 *
 * No proximity voice system exists yet (Week 6 - blocked on the Vivox plugin, see
 * docs/PROJECT_STATUS.md section 8), so IsAmplifying()/GetAmplifiedVoiceRadius() are a no-op
 * hook for now - the same "wire in real behavior once the dependency exists" pattern
 * UjinzzaGameUserSettings already uses for its audio sliders and UjinzzaDisguiseComponent uses
 * for face materials. Harmless today; a future voice component just needs to read these.
 */
UCLASS()
class JINZZA_API AjinzzaMegaphoneProp : public AjinzzaInteractableProp
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Megaphone")
	bool IsAmplifying() const { return bIsAmplifying; }

	/** How far the wielder's voice should carry while amplifying - read by a future proximity voice component. */
	UFUNCTION(BlueprintPure, Category = "Megaphone")
	float GetAmplifiedVoiceRadius() const { return bIsAmplifying ? AmplifiedVoiceRadius : 0.f; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnPropActivated_Implementation() override;

	/** Blueprint hook for a looping "amplifying" VFX/icon - distinct from the one-shot use effects the base class already fires. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Megaphone", meta = (DisplayName = "On Amplify State Changed"))
	void BP_OnAmplifyStateChanged(bool bNewIsAmplifying);

	UFUNCTION()
	void OnRep_IsAmplifying();

private:
	/** Voice radius while amplifying (cm). */
	UPROPERTY(EditAnywhere, Category = "Megaphone", meta = (ClampMin = 0, Units = "cm"))
	float AmplifiedVoiceRadius = 6000.f;

	UPROPERTY(ReplicatedUsing = OnRep_IsAmplifying)
	bool bIsAmplifying = false;
};
