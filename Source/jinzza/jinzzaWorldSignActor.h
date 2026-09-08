// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "jinzzaWorldSignActor.generated.h"

class UWidgetComponent;
class UjinzzaWorldSignWidget;

/**
 * A static, world-space text sign - the UWidgetComponent-based replacement for a plain
 * TextRenderActor, needed because UTextRenderComponent cannot render this project's Runtime-cached
 * Korean font at all (see UjinzzaWorldSignWidget's class comment). Used throughout Lvl_test for
 * every standalone sign (zone descriptions, section headers, the welcome sign).
 *
 * The component's front face (readable side) points along its own local +X axis, same convention
 * as everything else facing a booth's open mouth in Lvl_test - an actor yaw of -90 makes it face
 * world -Y, matching every prop/kiosk in that level.
 */
UCLASS()
class JINZZA_API AjinzzaWorldSignActor : public AActor
{
	GENERATED_BODY()

public:
	AjinzzaWorldSignActor();

	/** Text shown on the sign. Editable per-instance in the level. */
	UPROPERTY(EditAnywhere, Category = "Sign")
	FText SignText;

	/** Widget class to use - see UjinzzaWorldSignWidget. Left unset until a Widget Blueprint (e.g.
	 * WBP_WorldSign) exists, same "wire content later" pattern as AjinzzaInteractableProp's
	 * InteractionPromptWidgetClass. */
	UPROPERTY(EditAnywhere, Category = "Sign")
	TSubclassOf<UjinzzaWorldSignWidget> SignWidgetClass;

	/** World-space size of the sign in Unreal units - both the widget's draw/render-target
	 * resolution and its physical size, since the component runs at RelativeScale3D = 1 (1 Slate
	 * unit = 1 uu). */
	UPROPERTY(EditAnywhere, Category = "Sign")
	FVector2D SignSize = FVector2D(600.f, 220.f);

	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void BeginPlay() override;

private:
	void ApplySign();

	UPROPERTY(VisibleAnywhere, Category = "Sign")
	TObjectPtr<UWidgetComponent> SignWidgetComponent;
};
