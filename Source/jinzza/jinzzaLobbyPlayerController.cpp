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

	const FVector MyLocation = MyPawn->GetActorLocation();

	AjinzzaInteractableKiosk* Closest = nullptr;
	float ClosestDistSq = TNumericLimits<float>::Max();

	for (TActorIterator<AjinzzaInteractableKiosk> It(GetWorld()); It; ++It)
	{
		AjinzzaInteractableKiosk* Kiosk = *It;

		// Horizontal distance to the kiosk's colliding bounds (the closet / desk body), not 3D
		// distance to its origin: kiosk BPs sit on the floor ~1 m below the pawn's centre, the clock
		// hangs 3 m up the wall, and a scaled-up closet's collision keeps the pawn far from its
		// centre - with the old check those were unreachable. Sign widgets are skipped (their 5 m
		// quads collide on the UI profile and would inflate the box). Kiosks with no colliding
		// mesh (the floating gear / play button) fall back to their origin.
		FBox Bounds(ForceInit);
		TArray<UPrimitiveComponent*> Parts;
		Kiosk->GetComponents(Parts);
		for (const UPrimitiveComponent* Part : Parts)
		{
			if (Part->IsRegistered() && Part->IsCollisionEnabled() && !Part->IsA<UWidgetComponent>())
			{
				Bounds += Part->Bounds.GetBox();
			}
		}
		if (!Bounds.IsValid)
		{
			Bounds = FBox(Kiosk->GetActorLocation(), Kiosk->GetActorLocation());
		}
		FVector Probe = MyLocation;
		Probe.Z = FMath::Clamp(Probe.Z, Bounds.Min.Z, Bounds.Max.Z);
		const float DistSq = Bounds.ComputeSquaredDistanceToPoint(Probe);
		if (DistSq <= FMath::Square(Kiosk->InteractionRadius) && DistSq < ClosestDistSq)
		{
			Closest = Kiosk;
			ClosestDistSq = DistSq;
		}
	}

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
	if (NearbyKiosk)
	{
		NearbyKiosk->Interact(this);
	}
}
