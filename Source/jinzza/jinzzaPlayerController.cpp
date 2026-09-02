// Copyright Epic Games, Inc. All Rights Reserved.


#include "jinzzaPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "jinzzaCameraManager.h"
#include "jinzzaGameUserSettings.h"
#include "Blueprint/UserWidget.h"
#include "jinzza.h"
#include "Widgets/Input/SVirtualJoystick.h"

AjinzzaPlayerController::AjinzzaPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AjinzzaCameraManager::StaticClass();
}

void AjinzzaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	
	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(Logjinzza, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AjinzzaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(BuildRuntimeMappingContext(CurrentContext), 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(BuildRuntimeMappingContext(CurrentContext), 0);
				}
			}
		}
	}
	
}

bool AjinzzaPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

UInputMappingContext* AjinzzaPlayerController::BuildRuntimeMappingContext(UInputMappingContext* Source)
{
	if (!Source)
	{
		return nullptr;
	}

	UInputMappingContext* Runtime = DuplicateObject<UInputMappingContext>(Source, this);
	RuntimeMappingContexts.Add(Runtime);

	const UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return Runtime;
	}

	// Iterate the mappings as they existed on Source (Runtime starts as an identical copy), applying any
	// per-player key override on top of the duplicate so the shared .uasset is never mutated.
	for (const FEnhancedActionKeyMapping& Mapping : Source->GetMappings())
	{
		if (!Mapping.Action)
		{
			continue;
		}

		const FKey Rebind = Settings->GetKeyRebind(Mapping.Action->GetFName());
		if (Rebind.IsValid())
		{
			Runtime->UnmapKey(Mapping.Action, Mapping.Key);
			Runtime->MapKey(Mapping.Action, Rebind);
		}
	}

	return Runtime;
}
