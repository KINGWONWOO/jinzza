// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaMainMenuWidget.h"
#include "jinzza.h"
#include "UObject/ConstructorHelpers.h"

AjinzzaMenuPlayerController::AjinzzaMenuPlayerController()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> MainMenuWidgetBPClass(TEXT("/Game/JINZZA/UI/Widgets/WBP_MainMenu"));
	if (MainMenuWidgetBPClass.Succeeded())
	{
		MainMenuWidgetClass = MainMenuWidgetBPClass.Class;
	}
}

void AjinzzaMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	TSubclassOf<UUserWidget> WidgetClass = MainMenuWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UjinzzaMainMenuWidget::StaticClass();
	}

	MainMenuWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (MainMenuWidget)
	{
		MainMenuWidget->AddToViewport();
		MainMenuWidget->SetIsFocusable(true);

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
	else
	{
		UE_LOG(Logjinzza, Error, TEXT("Failed to create main menu widget."));
	}
}
