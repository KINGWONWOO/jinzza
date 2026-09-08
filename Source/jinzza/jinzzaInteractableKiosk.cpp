// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaInteractableKiosk.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void AjinzzaInteractableKiosk::EnterKioskUIMode(APlayerController* Interactor, UUserWidget* Widget)
{
	if (!Interactor)
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	if (Widget)
	{
		InputMode.SetWidgetToFocus(Widget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	Interactor->SetInputMode(InputMode);
	Interactor->bShowMouseCursor = true;

	// TEMP placeholder confirm sound for E-key kiosk interaction - swap for real SFX later.
	if (USoundBase* ConfirmSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/UISounds/LobbyPannelOpen__cut_1sec_.LobbyPannelOpen__cut_1sec_")))
	{
		UGameplayStatics::PlaySound2D(Interactor, ConfirmSound);
	}
}

void AjinzzaInteractableKiosk::ExitKioskUIMode(APlayerController* Interactor)
{
	if (!Interactor)
	{
		return;
	}

	FInputModeGameOnly InputMode;
	Interactor->SetInputMode(InputMode);
	Interactor->bShowMouseCursor = false;
}
