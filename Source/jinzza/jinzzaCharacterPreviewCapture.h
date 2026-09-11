// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaCharacterPreviewCapture.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class USceneCaptureComponent2D;
class UPointLightComponent;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;

/**
 * A temporary character mesh (SKM_Manny_Simple, same mesh the real playable character uses)
 * plus a SceneCaptureComponent2D rendering it into a runtime render target - the live "preview
 * on the left" for UjinzzaCustomizationWidget's Customization screen, and also usable as a
 * pre-placed level actor (e.g. in Lvl_MainMenu) for an ambient main-menu character preview.
 *
 * Two creation paths are both valid: UjinzzaCustomizationWidget spawns its own instance at
 * runtime (GetWorld()->SpawnActor, off in empty space) so the Customization screen's preview
 * works in ANY level without needing one hand-placed per level; a level designer can ALSO place
 * one directly (as originally done in Lvl_MainMenu) for a preview that's visible before the
 * Customization screen is even opened - UjinzzaMainMenuWidget::TryWireCharacterPreview finds
 * whichever instance exists via TActorIterator. Multiple simultaneous instances are fine, each
 * owns its own independent render target.
 *
 * The render target is created transiently at runtime (NewObject, not a content asset) - same
 * "runtime-transient object, no content asset needed yet" pattern UjinzzaGameUserSettings
 * already uses for its SoundClass/SoundMix objects.
 *
 * RefreshAppearance() applies the local player's saved Head/HairColor/Accessory customization to
 * PreviewMesh via the same JinzzaCustomization::ApplyToMesh helper the real character uses (see
 * jinzzaCustomizationApply.h), so the preview always matches what a live pawn would show. Called
 * once from BeginPlay and again by UjinzzaCustomizationWidget after every option change.
 */
UCLASS()
class JINZZA_API AjinzzaCharacterPreviewCapture : public AActor
{
	GENERATED_BODY()

public:
	AjinzzaCharacterPreviewCapture();

	/** The texture UjinzzaMainMenuWidget's CharacterPreviewImage / UjinzzaCustomizationWidget's CharacterPreviewImage should show - valid only after BeginPlay. */
	UFUNCTION(BlueprintPure, Category = "Preview")
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	/** Re-applies the local player's current UjinzzaGameUserSettings appearance to PreviewMesh. */
	void RefreshAppearance();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewMesh;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USceneCaptureComponent2D> Capture;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UPointLightComponent> FillLight;

	UPROPERTY(EditAnywhere, Category = "Preview")
	FIntPoint RenderTargetSize = FIntPoint(512, 768);

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	/** Per-preview customization state, mirroring UjinzzaCharacterCustomizationComponent's own fields - see JinzzaCustomization::ApplyToMesh. */
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
