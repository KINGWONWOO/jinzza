// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaLobbyWidget.h"
#include "jinzzaRoomSettingsKiosk.h"
#include "EngineUtils.h"
#include "Components/InputComponent.h"

namespace
{
	constexpr float KioskCheckInterval = 0.2f;
}

void AjinzzaLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	LobbyWidget = CreateWidget<UUserWidget>(this, UjinzzaLobbyWidget::StaticClass());
	if (LobbyWidget)
	{
		LobbyWidget->AddToViewport();
		LobbyWidget->SetIsFocusable(true);
		bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(LobbyWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}

	GetWorldTimerManager().SetTimer(KioskCheckTimerHandle, this, &AjinzzaLobbyPlayerController::CheckForNearbyKiosk, KioskCheckInterval, true);
}

void AjinzzaLobbyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AjinzzaLobbyPlayerController::OnInteractPressed);
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

	AjinzzaRoomSettingsKiosk* Closest = nullptr;
	float ClosestDistSq = TNumericLimits<float>::Max();

	for (TActorIterator<AjinzzaRoomSettingsKiosk> It(GetWorld()); It; ++It)
	{
		AjinzzaRoomSettingsKiosk* Kiosk = *It;
		const float DistSq = FVector::DistSquared(MyLocation, Kiosk->GetActorLocation());
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
