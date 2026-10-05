// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaMainMenuWidget.h"
#include "jinzzaMenuBackgroundCharacter.h"
#include "jinzzaMenuCameraRig.h"
#include "jinzzaMenuSceneDirector.h"
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
		// Placed into each backdrop scene by the scene director below - spawn location doesn't matter.
		BackgroundCharacter = World->SpawnActor<AjinzzaMenuBackgroundCharacter>(FVector(0.f, 0.f, 5000.f), FRotator::ZeroRotator);
	}

	AjinzzaMenuCameraRig* CameraRig = nullptr;
	for (TActorIterator<AjinzzaMenuCameraRig> It(World); It; ++It)
	{
		CameraRig = *It;
		break;
	}
	if (!CameraRig)
	{
		// Moved/aimed every frame by the scene director below.
		CameraRig = World->SpawnActor<AjinzzaMenuCameraRig>(FVector::ZeroVector, FRotator::ZeroRotator);
	}

	if (CameraRig)
	{
		SetViewTarget(CameraRig);
	}

	// The backdrop scenes are built far out along +X (from X = 20000), clear of Lvl_MainMenu's own
	// floor/walls and the AjinzzaCharacterPreviewCapture at the origin.
	AjinzzaMenuSceneDirector* Director = nullptr;
	for (TActorIterator<AjinzzaMenuSceneDirector> It(World); It; ++It)
	{
		Director = *It;
		break;
	}
	if (!Director)
	{
		Director = World->SpawnActor<AjinzzaMenuSceneDirector>(FVector(20000.f, 0.f, 0.f), FRotator::ZeroRotator);
	}
	if (Director)
	{
		Director->Start(BackgroundCharacter, CameraRig, this);
	}
}
