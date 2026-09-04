// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaCharacterPreviewCapture.generated.h"

class USkeletalMeshComponent;
class USceneCaptureComponent2D;
class UPointLightComponent;
class UTextureRenderTarget2D;

/**
 * Placed in Lvl_MainMenu: a temporary character mesh (SK_Mannequin by default) plus a
 * SceneCaptureComponent2D rendering it into a runtime render target, so
 * UjinzzaMainMenuWidget's CharacterPreviewImage can show a live character preview on the main
 * menu without needing the "no pawn" AjinzzaMenuGameMode to have a camera/pawn at all - the
 * capture renders independently of whatever (if anything) the main viewport camera shows.
 *
 * The render target is created transiently at runtime (NewObject, not a content asset) - same
 * "runtime-transient object, no content asset needed yet" pattern UjinzzaGameUserSettings
 * already uses for its SoundClass/SoundMix objects.
 */
UCLASS()
class JINZZA_API AjinzzaCharacterPreviewCapture : public AActor
{
	GENERATED_BODY()

public:
	AjinzzaCharacterPreviewCapture();

	/** The texture UjinzzaMainMenuWidget's CharacterPreviewImage should show - valid only after BeginPlay. */
	UFUNCTION(BlueprintPure, Category = "Preview")
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

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
};
