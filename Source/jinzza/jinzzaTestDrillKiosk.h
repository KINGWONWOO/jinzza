// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaInteractableKiosk.h"
#include "jinzzaTestGameMode.h"
#include "jinzzaTestDrillKiosk.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;

/**
 * Lvl_test station that starts one match drill (AjinzzaTestGameMode::StartDrill) - walk up, press Interact.
 * Pressing it again restarts that drill; the Reset station stops whatever is running. A pedestal with a
 * colored button (color = drill) and a floating title that turns toward you.
 */
UCLASS()
class JINZZA_API AjinzzaTestDrillKiosk : public AjinzzaInteractableKiosk
{
	GENERATED_BODY()

public:
	AjinzzaTestDrillKiosk();

	UPROPERTY(EditAnywhere, Category = "Drill")
	EJinzzaTestDrill Drill = EJinzzaTestDrill::SelfIntroduction;

	/** Floating title above the station (Korean is fine - it's drawn with UMG). Empty = a default per drill. */
	UPROPERTY(EditAnywhere, Category = "Drill")
	FText Title;

	virtual FText GetInteractionPrompt() const override;
	virtual void Interact(APlayerController* Interactor) override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	void ApplyLook();
	FText GetTitleText() const;

	UPROPERTY(VisibleAnywhere, Category = "Drill")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Drill")
	TObjectPtr<UStaticMeshComponent> Pedestal;

	UPROPERTY(VisibleAnywhere, Category = "Drill")
	TObjectPtr<UStaticMeshComponent> Button;

	UPROPERTY(VisibleAnywhere, Category = "Drill")
	TObjectPtr<UWidgetComponent> Label;
};
