// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaLobbyWidget.h"

void AjinzzaLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	LobbyWidget = CreateWidget<UUserWidget>(this, UjinzzaLobbyWidget::StaticClass());
	if (LobbyWidget)
	{
		LobbyWidget->AddToViewport();
		LobbyWidget->SetIsFocusable(true);
		bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(LobbyWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
}
