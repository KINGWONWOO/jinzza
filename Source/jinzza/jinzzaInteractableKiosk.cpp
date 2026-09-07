// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaInteractableKiosk.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

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
