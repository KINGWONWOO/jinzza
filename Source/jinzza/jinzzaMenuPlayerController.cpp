// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaMainMenuWidget.h"
#include "jinzzaMenuBackgroundCharacter.h"
#include "jinzzaMenuCameraRig.h"
#include "jinzza.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"

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

	SetupMenuBackgroundScene();
}

void AjinzzaMenuPlayerController::SetupMenuBackgroundScene()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AjinzzaMenuBackgroundCharacter* BackgroundCharacter = nullptr;
	for (TActorIterator<AjinzzaMenuBackgroundCharacter> It(World); It; ++It)
	{
		BackgroundCharacter = *It;
		break;
	}
	if (!BackgroundCharacter)
	{
		// Faces back toward the origin (Yaw 180), where the camera rig looks from - see below.
		World->SpawnActor<AjinzzaMenuBackgroundCharacter>(FVector(500.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f));
	}

	AjinzzaMenuCameraRig* CameraRig = nullptr;
	for (TActorIterator<AjinzzaMenuCameraRig> It(World); It; ++It)
	{
		CameraRig = *It;
		break;
	}
	if (!CameraRig)
	{
		// Faces +X (ZeroRotator), toward the character spawned above.
		CameraRig = World->SpawnActor<AjinzzaMenuCameraRig>(FVector(0.f, 0.f, 150.f), FRotator::ZeroRotator);
	}

	if (CameraRig)
	{
		SetViewTarget(CameraRig);
	}
}
