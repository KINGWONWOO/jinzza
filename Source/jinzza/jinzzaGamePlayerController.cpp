// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGamePlayerController.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaGameEndWidget.h"

void AjinzzaGamePlayerController::Client_ReceiveRoleAssignment_Implementation(EJinzzaPartyRole InRole, APlayerState* InRealOne)
{
	LocalRole = InRole;
	KnownRealOne = InRealOne;
}

void AjinzzaGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	TSubclassOf<UUserWidget> WidgetClass = GameEndWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UjinzzaGameEndWidget::StaticClass();
	}

	GameEndWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (GameEndWidget)
	{
		GameEndWidget->AddToViewport();
		GameEndWidget->SetIsFocusable(true);
		bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(GameEndWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
}
