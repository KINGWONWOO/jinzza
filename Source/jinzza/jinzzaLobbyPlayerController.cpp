// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaLobbyWidget.h"
#include "jinzzaInteractableKiosk.h"
#include "jinzzaInteractHighlight.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Components/WidgetComponent.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputComponent.h"
#include "jinzzaInputKeys.h"
#include "jinzzaLobbyGameMode.h"
#include "jinzzaLoadingScreenSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float KioskCheckInterval = 0.2f;
}

AjinzzaLobbyPlayerController::AjinzzaLobbyPlayerController()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> LobbyWidgetBPClass(TEXT("/Game/JINZZA/UI/Widgets/WBP_Lobby"));
	if (LobbyWidgetBPClass.Succeeded())
	{
		LobbyWidgetClass = LobbyWidgetBPClass.Class;
	}
}

void AjinzzaLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	TSubclassOf<UUserWidget> WidgetClass = LobbyWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UjinzzaLobbyWidget::StaticClass();
	}

	LobbyWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (LobbyWidget)
	{
		// Purely a display HUD now (room info + player count) - no buttons live here anymore
		// (Invite/Start Match moved to walk-up-to kiosks, see AjinzzaFriendInviteKiosk/
		// AjinzzaStartMatchKiosk), so it doesn't need keyboard focus or a visible cursor. Default
		// to normal hidden-cursor/Game-only input so WASD look/move work immediately on entering
		// the lobby - kiosks switch into UI mode themselves while their panel is open (see
		// AjinzzaInteractableKiosk::EnterKioskUIMode/ExitKioskUIMode).
		LobbyWidget->AddToViewport();
		bShowMouseCursor = false;

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
	}

	// Every kiosk (and the clock) keeps a white outline for the whole lobby, so players can see
	// at a glance what they can walk up to - not just the one in E range.
	for (TActorIterator<AjinzzaInteractableKiosk> It(GetWorld()); It; ++It)
	{
		JinzzaHighlight::SetHighlighted(*It, true);
	}

	GetWorldTimerManager().SetTimer(KioskCheckTimerHandle, this, &AjinzzaLobbyPlayerController::CheckForNearbyKiosk, KioskCheckInterval, true);
}

void AjinzzaLobbyPlayerController::Client_PrepareForMatch_Implementation(const FString& MapPath)
{
	UjinzzaLoadingScreenSubsystem* Loading = GetGameInstance() ? GetGameInstance()->GetSubsystem<UjinzzaLoadingScreenSubsystem>() : nullptr;
	if (!Loading)
	{
		Server_ReportMatchPreloaded();
		return;
	}

	TWeakObjectPtr<AjinzzaLobbyPlayerController> WeakThis(this);
	Loading->BeginMatchPreload(MapPath, [WeakThis]()
	{
		if (AjinzzaLobbyPlayerController* PC = WeakThis.Get())
		{
			PC->Server_ReportMatchPreloaded();
		}
	});
}

void AjinzzaLobbyPlayerController::Server_ReportMatchPreloaded_Implementation()
{
	if (AjinzzaLobbyGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AjinzzaLobbyGameMode>() : nullptr)
	{
		GameMode->NotifyMatchPreloaded(this);
	}
}

void AjinzzaLobbyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// IA_Interact (not a hardcoded key), so kiosks follow the player's Interact rebind like props do.
	// The pawn binds the same action for props; both handlers run, and each is a no-op with nothing in range.
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (UInputAction* InteractAction = JinzzaInput::GetInteractAction(); EnhancedInputComponent && InteractAction)
	{
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AjinzzaLobbyPlayerController::OnInteractPressed);
	}
}

void AjinzzaLobbyPlayerController::CheckForNearbyKiosk()
{
	if (!IsLocalController())
	{
		return;
	}

	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}

	AjinzzaInteractableKiosk* Closest = AjinzzaInteractableKiosk::FindNearby(MyPawn);

	if (Closest != NearbyKiosk)
	{
		NearbyKiosk = Closest;

		if (UjinzzaLobbyWidget* Lobby = Cast<UjinzzaLobbyWidget>(LobbyWidget))
		{
			Lobby->SetInteractionPrompt(Closest ? Closest->GetInteractionPrompt() : FText::GetEmpty());
		}
	}
}

void AjinzzaLobbyPlayerController::OnInteractPressed()
{
	if (NearbyKiosk && !IsPauseMenuOpen())
	{
		NearbyKiosk->Interact(this);
	}
}
