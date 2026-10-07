// Copyright Epic Games, Inc. All Rights Reserved.


#include "jinzzaPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "UObject/SoftObjectPath.h"
#include "jinzzaCameraManager.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaInputKeys.h"
#include "jinzzaPartyPlayerState.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaPauseMenuWidget.h"
#include "jinzzaChatWidget.h"
#include "jinzzaChatBoardComponent.h"
#include "jinzzaLoadingScreenSubsystem.h"
#include "Engine/GameInstance.h"
#include "Components/Widget.h"
#include "Engine/World.h"
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

	
	if (IsLocalPlayerController())
	{
		ChatWidget = CreateWidget<UjinzzaChatWidget>(this, UjinzzaChatWidget::StaticClass());
		if (ChatWidget)
		{
			// Above the HUD, below the ESC menu (100) and the loading screen.
			ChatWidget->AddToViewport(40);
		}
	}

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
		CreatePushToTalkAction();
		CreatePauseMenuAction();
		AddRuntimeMappingContexts();

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			EnhancedInputComponent->BindAction(PushToTalkAction, ETriggerEvent::Started, this, &AjinzzaPlayerController::OnPushToTalkPressed);
			EnhancedInputComponent->BindAction(PushToTalkAction, ETriggerEvent::Completed, this, &AjinzzaPlayerController::OnPushToTalkReleased);
			EnhancedInputComponent->BindAction(PushToTalkAction, ETriggerEvent::Canceled, this, &AjinzzaPlayerController::OnPushToTalkReleased);
			EnhancedInputComponent->BindAction(PauseMenuAction, ETriggerEvent::Started, this, &AjinzzaPlayerController::OnPauseMenuPressed);
			EnhancedInputComponent->BindAction(ChatAction, ETriggerEvent::Started, this, &AjinzzaPlayerController::OnChatPressed);
		}

		GetWorldTimerManager().SetTimer(VoiceUpdateTimerHandle, this, &AjinzzaPlayerController::UpdateVoiceTransmission, 0.25f, true);
	}
}

void AjinzzaPlayerController::AddRuntimeMappingContexts()
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem)
	{
		return;
	}

	for (UInputMappingContext* Existing : RuntimeMappingContexts)
	{
		if (Existing)
		{
			Subsystem->RemoveMappingContext(Existing);
		}
	}
	RuntimeMappingContexts.Reset();

	// Add Input Mapping Context
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

	if (PushToTalkAction)
	{
		const UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
		const FKey Rebind = Settings ? Settings->GetKeyRebind(JinzzaInput::GetPushToTalkActionName()) : EKeys::Invalid;

		UInputMappingContext* VoiceContext = NewObject<UInputMappingContext>(this);
		VoiceContext->MapKey(PushToTalkAction, Rebind.IsValid() ? Rebind : JinzzaInput::GetDefaultPushToTalkKey());
		RuntimeMappingContexts.Add(VoiceContext);
		Subsystem->AddMappingContext(VoiceContext, 0);
	}

	if (PauseMenuAction)
	{
		UInputMappingContext* PauseContext = NewObject<UInputMappingContext>(this);
		PauseContext->MapKey(PauseMenuAction, EKeys::Escape);
		PauseContext->MapKey(PauseMenuAction, EKeys::Gamepad_Special_Right);
		if (ChatAction)
		{
			PauseContext->MapKey(ChatAction, EKeys::Enter);
		}
		RuntimeMappingContexts.Add(PauseContext);
		Subsystem->AddMappingContext(PauseContext, 0);
	}
}

void AjinzzaPlayerController::RefreshKeyBindings()
{
	if (IsLocalPlayerController())
	{
		AddRuntimeMappingContexts();
	}
}

void AjinzzaPlayerController::CreatePushToTalkAction()
{
	if (!PushToTalkAction)
	{
		// ValueType defaults to Boolean - a plain held/released button.
		PushToTalkAction = NewObject<UInputAction>(this, JinzzaInput::GetPushToTalkActionName());
	}
}

void AjinzzaPlayerController::CreatePauseMenuAction()
{
	if (!PauseMenuAction)
	{
		PauseMenuAction = NewObject<UInputAction>(this, TEXT("IA_PauseMenu"));
	}
	if (!ChatAction)
	{
		ChatAction = NewObject<UInputAction>(this, TEXT("IA_Chat"));
	}
}

void AjinzzaPlayerController::OnPauseMenuPressed()
{
	// Hidden under the loading screen it would still grab input.
	if (!IsLoadingScreenUp())
	{
		TogglePauseMenu();
	}
}

bool AjinzzaPlayerController::IsLoadingScreenUp() const
{
	const UjinzzaLoadingScreenSubsystem* Loading = GetGameInstance() ? GetGameInstance()->GetSubsystem<UjinzzaLoadingScreenSubsystem>() : nullptr;
	return Loading && Loading->IsShowing();
}

void AjinzzaPlayerController::TogglePauseMenu()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (PauseMenuWidget)
	{
		ClosePauseMenu();
		return;
	}

	if (!CanOpenPauseMenu())
	{
		return;
	}

	PauseMenuWidget = CreateWidget<UjinzzaPauseMenuWidget>(this, UjinzzaPauseMenuWidget::StaticClass());
	if (!PauseMenuWidget)
	{
		return;
	}

	PauseMenuWidget->OnCloseRequested.AddUObject(this, &AjinzzaPlayerController::ClosePauseMenu);
	// Above the HUD/lobby widgets (Z 0) and any open prompts.
	PauseMenuWidget->AddToViewport(100);

	// Game-and-UI rather than UI-only so ESC still reaches OnPauseMenuPressed (and closes the menu) even if a
	// click on the game view took keyboard focus away from the widget; movement and look are blocked instead.
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
}

void AjinzzaPlayerController::ClosePauseMenu()
{
	if (!PauseMenuWidget)
	{
		return;
	}

	PauseMenuWidget->RemoveFromParent();
	PauseMenuWidget = nullptr;

	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	RestoreGameplayInputMode();
}

void AjinzzaPlayerController::OnChatPressed()
{
	// No character (e.g. spectating) = no board to write on.
	if (ChatWidget && !ChatWidget->IsInputOpen() && !IsPauseMenuOpen() && !IsLoadingScreenUp() && CanOpenPauseMenu() && CanUseChat() && GetChatBoard())
	{
		ChatWidget->OpenInput();
	}
}

void AjinzzaPlayerController::EnterChatInputMode(UWidget* FocusTarget)
{
	FInputModeUIOnly InputMode;
	if (FocusTarget)
	{
		InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AjinzzaPlayerController::ExitChatInputMode()
{
	if (!IsPauseMenuOpen())
	{
		RestoreGameplayInputMode();
	}
}

void AjinzzaPlayerController::Server_SendChatMessage_Implementation(const FString& Text)
{
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	FString Clean = Text.Replace(TEXT("\r"), TEXT(" ")).Replace(TEXT("\n"), TEXT(" ")).TrimStartAndEnd();
	Clean.LeftInline(JinzzaChat::MaxMessageLength);
	if (Clean.IsEmpty() || Now - LastChatMessageTime < JinzzaChat::MinSecondsBetweenMessages)
	{
		// Rejected - don't leave the board raised and blank.
		if (UjinzzaChatBoardComponent* Board = GetChatBoard())
		{
			Board->ServerCancelWriting();
		}
		return;
	}
	LastChatMessageTime = Now;

	if (RouteChatMessage(Clean))
	{
		return;
	}

	// Who can see it (ghost boards only to ghosts) is decided on each viewer's machine by the board itself.
	if (UjinzzaChatBoardComponent* Board = GetChatBoard())
	{
		Board->ServerReveal(Clean);
	}
}

void AjinzzaPlayerController::Server_SetChatWriting_Implementation(bool bWriting)
{
	if (bWriting && !ShouldUseChatBoard())
	{
		return;
	}
	if (UjinzzaChatBoardComponent* Board = GetChatBoard())
	{
		if (bWriting)
		{
			Board->ServerStartWriting();
		}
		else
		{
			Board->ServerCancelWriting();
		}
	}
}

void AjinzzaPlayerController::UpdateChatPreview(const FString& Text)
{
	if (UjinzzaChatBoardComponent* Board = GetChatBoard())
	{
		Board->SetLocalPreviewText(Text);
	}
}

void AjinzzaPlayerController::CancelChatInput()
{
	if (ChatWidget && ChatWidget->IsInputOpen())
	{
		ChatWidget->CancelInput();
	}
}

UjinzzaChatBoardComponent* AjinzzaPlayerController::GetChatBoard() const
{
	return GetPawn() ? GetPawn()->FindComponentByClass<UjinzzaChatBoardComponent>() : nullptr;
}

void AjinzzaPlayerController::RestoreGameplayInputMode()
{
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void AjinzzaPlayerController::OnPushToTalkPressed()
{
	bPushToTalkHeld = true;
	UpdateVoiceTransmission();
}

void AjinzzaPlayerController::OnPushToTalkReleased()
{
	bPushToTalkHeld = false;
	UpdateVoiceTransmission();
}

void AjinzzaPlayerController::ApplyMicInputMode()
{
	UpdateVoiceTransmission();
}

void AjinzzaPlayerController::ClientEnableNetworkVoice_Implementation(bool bEnable)
{
	// Deliberately not calling Super: it would start/stop talking from the server-wide bRequiresPushToTalk
	// instead of this player's own setting. The engine's call resets the mic, so resync from scratch.
	bVoiceReady = true;
	bTransmittingVoice = false;
	ToggleSpeaking(false);
	UpdateVoiceTransmission();
}

void AjinzzaPlayerController::UpdateVoiceTransmission()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	bool bWantTransmit = false;
	const UWorld* World = GetWorld();
	if (bVoiceReady && World && World->GetNetMode() != NM_Standalone)
	{
		const UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
		const bool bOpenMic = Settings && Settings->GetMicInputMode() == EJinzzaMicInputMode::OpenMic;
		bWantTransmit = bOpenMic || bPushToTalkHeld;

		// Ghosts can't talk (design doc 8-2). Other players' machines also silence a ghost's voice on their side
		// (UjinzzaProximityVoiceComponent), so this is just to stop sending it at all.
		if (const AjinzzaPartyPlayerState* PartyState = GetPlayerState<AjinzzaPartyPlayerState>())
		{
			bWantTransmit &= !PartyState->IsGhost();
		}
		bWantTransmit &= !IsVoiceBlocked();
	}

	if (bWantTransmit != bTransmittingVoice)
	{
		bTransmittingVoice = bWantTransmit;
		ToggleSpeaking(bWantTransmit);
	}
}

void AjinzzaPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(VoiceUpdateTimerHandle);
	if (bTransmittingVoice)
	{
		bTransmittingVoice = false;
		ToggleSpeaking(false);
	}

	Super::EndPlay(EndPlayReason);
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

	// Unique name - RefreshKeyBindings rebuilds these while the previous copies still exist under the same outer.
	UInputMappingContext* Runtime = DuplicateObject<UInputMappingContext>(Source, this, MakeUniqueObjectName(this, UInputMappingContext::StaticClass(), Source->GetFName()));
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
