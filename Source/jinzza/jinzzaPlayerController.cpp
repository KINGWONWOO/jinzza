// Copyright Epic Games, Inc. All Rights Reserved.


#include "jinzzaPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "UObject/SoftObjectPath.h"
#include "jinzzaCameraManager.h"
#include "jinzzaGameUserSettings.h"
#include "Blueprint/UserWidget.h"
#include "jinzza.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "UObject/ConstructorHelpers.h"

AjinzzaPlayerController::AjinzzaPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AjinzzaCameraManager::StaticClass();

	// Hardcoded as C++ constructor defaults (rather than relying only on a Blueprint's Class
	// Defaults panel) so every subclass - AjinzzaLobbyPlayerController and
	// AjinzzaGamePlayerController included, neither of which has its own Blueprint asset - gets
	// working Enhanced Input out of the box. A subclass's own Blueprint (if one exists) can still
	// override these arrays entirely from its Class Defaults panel as usual.
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultIMCFinder(TEXT("/Game/JINZZA/Input/IMC_Default.IMC_Default"));
	if (DefaultIMCFinder.Succeeded())
	{
		DefaultMappingContexts.Add(DefaultIMCFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> SprintIMCFinder(TEXT("/Game/JINZZA/Input/IMC_Sprint.IMC_Sprint"));
	if (SprintIMCFinder.Succeeded())
	{
		DefaultMappingContexts.Add(SprintIMCFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MouseLookIMCFinder(TEXT("/Game/JINZZA/Input/IMC_MouseLook.IMC_MouseLook"));
	if (MouseLookIMCFinder.Succeeded())
	{
		MobileExcludedMappingContexts.Add(MouseLookIMCFinder.Object);
	}
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

	// Iterate the mappings as they existed on Source (Runtime starts as an identical copy), applying any
	// per-player key override on top of the duplicate so the shared .uasset is never mutated.
	if (const UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
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
	}

	// IA_EmoteWheel is authored on E in IMC_Default, colliding with IA_Interact (also E - kiosks/
	// props) - holding E to interact was also opening the emote wheel. The Mappings array on the
	// shared .uasset isn't reliably readable/editable through this project's editor-automation
	// tooling (confirmed repeatedly - see docs/unreal_mcp_gotchas.md), so correct it here on the
	// runtime duplicate instead, same pattern as the per-player rebind loop above. This is a
	// default-key fix, not a per-player preference, so it runs unconditionally rather than going
	// through UjinzzaGameUserSettings::GetKeyRebind.
	static const FSoftObjectPath EmoteWheelActionPath(TEXT("/Game/JINZZA/Input/Actions/IA_EmoteWheel.IA_EmoteWheel"));
	if (UInputAction* EmoteWheelAction = Cast<UInputAction>(EmoteWheelActionPath.TryLoad()))
	{
		TArray<FKey> KeysToUnmap;
		for (const FEnhancedActionKeyMapping& Mapping : Runtime->GetMappings())
		{
			if (Mapping.Action == EmoteWheelAction && Mapping.Key != EKeys::Tab)
			{
				KeysToUnmap.Add(Mapping.Key);
			}
		}
		for (const FKey& Key : KeysToUnmap)
		{
			Runtime->UnmapKey(EmoteWheelAction, Key);
		}

		const bool bAlreadyOnTab = Runtime->GetMappings().ContainsByPredicate([EmoteWheelAction](const FEnhancedActionKeyMapping& M)
		{
			return M.Action == EmoteWheelAction && M.Key == EKeys::Tab;
		});
		if (!bAlreadyOnTab)
		{
			Runtime->MapKey(EmoteWheelAction, EKeys::Tab);
		}
	}

	return Runtime;
}
