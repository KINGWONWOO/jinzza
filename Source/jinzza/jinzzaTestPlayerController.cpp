// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaTestPlayerController.h"
#include "jinzzaInteractableKiosk.h"
#include "jinzzaInteractHighlight.h"
#include "jinzzaInputKeys.h"
#include "jinzzaTestHudWidget.h"
#include "EnhancedInputComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	constexpr float TestKioskCheckInterval = 0.2f;
}

void AjinzzaTestPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	TestHud = CreateWidget<UjinzzaTestHudWidget>(this, UjinzzaTestHudWidget::StaticClass());
	if (TestHud)
	{
		TestHud->AddToViewport(20);
	}

	GetWorldTimerManager().SetTimer(KioskCheckTimerHandle, this, &AjinzzaTestPlayerController::CheckForNearbyKiosk, TestKioskCheckInterval, true);
}

void AjinzzaTestPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// IA_Interact, like AjinzzaLobbyPlayerController - the pawn binds it too (props); each handler is a no-op
	// with nothing in range.
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (UInputAction* InteractAction = JinzzaInput::GetInteractAction(); EnhancedInputComponent && InteractAction)
	{
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AjinzzaTestPlayerController::OnInteractPressed);
	}
}

void AjinzzaTestPlayerController::CheckForNearbyKiosk()
{
	// Same as the lobby: every kiosk keeps a white outline, so you can see what you can walk up to.
	for (TActorIterator<AjinzzaInteractableKiosk> It(GetWorld()); It; ++It)
	{
		if (!HighlightedKiosks.Contains(*It))
		{
			HighlightedKiosks.Add(*It);
			JinzzaHighlight::SetHighlighted(*It, true);
		}
	}

	AjinzzaInteractableKiosk* Closest = AjinzzaInteractableKiosk::FindNearby(GetPawn());
	if (Closest != NearbyKiosk)
	{
		NearbyKiosk = Closest;
		if (TestHud)
		{
			TestHud->SetInteractionPrompt(Closest ? Closest->GetInteractionPrompt() : FText::GetEmpty());
		}
	}
}

void AjinzzaTestPlayerController::OnInteractPressed()
{
	if (NearbyKiosk && !IsPauseMenuOpen())
	{
		NearbyKiosk->Interact(this);
	}
}

void AjinzzaTestPlayerController::Server_StartDrill_Implementation(EJinzzaTestDrill Drill)
{
	if (AjinzzaTestGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AjinzzaTestGameMode>() : nullptr)
	{
		GameMode->StartDrill(Drill, this);
	}
}
