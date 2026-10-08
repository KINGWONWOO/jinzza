// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaInteractableKiosk.h"
#include "GameFramework/Pawn.h"
#include "Components/WidgetComponent.h"
#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"
#include "jinzzaInteractHighlight.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AjinzzaInteractableKiosk::AjinzzaInteractableKiosk()
{
	FaceCamera = CreateDefaultSubobject<UjinzzaFaceCameraComponent>(TEXT("FaceCamera"));
}

AjinzzaInteractableKiosk* AjinzzaInteractableKiosk::FindNearby(const APawn* Pawn)
{
	UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	const FVector MyLocation = Pawn->GetActorLocation();
	AjinzzaInteractableKiosk* Closest = nullptr;
	float ClosestDistSq = TNumericLimits<float>::Max();

	for (TActorIterator<AjinzzaInteractableKiosk> It(World); It; ++It)
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
	return Closest;
}

void AjinzzaInteractableKiosk::EnterKioskUIMode(APlayerController* Interactor, UUserWidget* Widget)
{
	if (!Interactor)
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	if (Widget)
	{
		InputMode.SetWidgetToFocus(Widget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	Interactor->SetInputMode(InputMode);
	Interactor->bShowMouseCursor = true;

	// TEMP placeholder confirm sound for E-key kiosk interaction - swap for real SFX later.
	if (USoundBase* ConfirmSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/UISounds/LobbyPannelOpen__cut_1sec_.LobbyPannelOpen__cut_1sec_")))
	{
		UGameplayStatics::PlaySound2D(Interactor, ConfirmSound);
	}
}

void AjinzzaInteractableKiosk::ExitKioskUIMode(APlayerController* Interactor)
{
	if (!Interactor)
	{
		return;
	}

	FInputModeGameOnly InputMode;
	Interactor->SetInputMode(InputMode);
	Interactor->bShowMouseCursor = false;
}
