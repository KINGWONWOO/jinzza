// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "jinzzaCharacterCustomizationComponent.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * Applies the local player's saved appearance (UjinzzaGameUserSettings::GetHeadStyle/HairColor/
 * TopStyle/EyebrowsStyle/EyesStyle) to the owning character. This is the
 * "UCharacterCustomizationComponent" the design doc's class list (section 11) already planned
 * for - see jinzzaDisguiseComponent.h's class comment, which called out that it didn't exist yet.
 *
 * No real head/hair/top/eyebrow/eye content exists yet (see jinzzaCustomizationTypes.h), so
 * RefreshCustomization() degrades to a harmless no-op past reading the saved values, same
 * pattern UjinzzaDisguiseComponent already uses for its face material and UjinzzaGameUserSettings
 * uses for audio it has no SoundClass content for. Once real meshes/materials exist per slot,
 * this is the one place that needs to change to actually apply them.
 *
 * Only ever meaningful for the locally-controlled character - RefreshCustomization() is a
 * client-local cosmetic refresh, called from BeginPlay and again whenever the player changes
 * something in UjinzzaCustomizationWidget (opened from the main menu or from
 * AjinzzaWardrobeKiosk in the lobby - both write to the same UjinzzaGameUserSettings).
 *
 * Head/HairColor/Accessory now have real (placeholder) content to apply, on the same
 * M_Face_Master/"FaceIndex" contract UjinzzaDisguiseComponent already uses (slot 0 of GetMesh(),
 * so a round disguise assigned later simply overwrites this base look - see that class comment)
 * plus lazily-created hair/accessory meshes socketed to the skeleton's "Head" socket, tinted via
 * M_Hair_Master's "HairColor" param. Top/Eyebrows/Eyes still have no matching content and stay
 * no-ops. The actual material-swap/socket-mesh work is shared with the Customization screen's
 * live preview via JinzzaCustomization::ApplyToMesh (jinzzaCustomizationApply.h) - see that
 * file's comment for the full contract, including the deferred Steam-avatar-on-face plan.
 */
UCLASS(ClassGroup = (Party), meta = (BlueprintSpawnableComponent))
class JINZZA_API UjinzzaCharacterCustomizationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Customization")
	void RefreshCustomization();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicFaceMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> HairMeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicHairMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> AccessoryMeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicAccessoryMaterial;
};
