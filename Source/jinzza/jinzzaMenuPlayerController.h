// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "jinzzaMenuPlayerController.generated.h"

class UUserWidget;

/** Spawns and displays the main menu widget, and puts input into UI-only mode. */
UCLASS()
class JINZZA_API AjinzzaMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AjinzzaMenuPlayerController();

	/** Widget class to show. Defaults to UjinzzaMainMenuWidget if left unset (WBP_MainMenu if it exists, else the raw C++ class). */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> MainMenuWidget;
};
