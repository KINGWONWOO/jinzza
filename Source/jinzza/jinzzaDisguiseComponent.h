// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "jinzzaDisguiseComponent.generated.h"

class UMaterialInstanceDynamic;

/**
 * Applies the owning character's disguised face (AjinzzaPartyPlayerState::GetFaceType) to its
 * third-person mesh (the one other players see - AjinzzaCharacter hides GetMesh() from its own
 * owner) via a dynamic material instance, driving a "FaceIndex" scalar parameter (0=none,
 * 1/2/3=A/B/C). MI_Face_A/B/C swapping (design doc section 12/13-2) doesn't exist as content
 * yet, so until a material with that parameter is authored this degrades to a harmless no-op -
 * same pattern UjinzzaGameUserSettings already uses for audio it has no SoundClass assets for.
 * A ghosted character (AjinzzaPartyPlayerState::IsGhost()) reverts to no override, matching the
 * design doc's "위장 해제" (disguise removed on elimination) - full restoration of the player's
 * own lobby-customized appearance is future work (UCharacterCustomizationComponent, section 11,
 * doesn't exist yet).
 *
 * RefreshDisguise() is called both from BeginPlay (pulls whatever role/face state already exists
 * on the PlayerState when this pawn spawns) and pushed from AjinzzaPartyPlayerState whenever its
 * disguise fields change (the common case: role assignment happens after the pawn already
 * exists, mid-match) - see AjinzzaPartyPlayerState::OnRep_DisguiseChanged.
 */
UCLASS(ClassGroup = (Party), meta = (BlueprintSpawnableComponent))
class JINZZA_API UjinzzaDisguiseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UjinzzaDisguiseComponent();

	void RefreshDisguise();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicFaceMaterial;
};
