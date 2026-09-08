// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGamePlayerController.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaGameEndWidget.h"
#include "jinzzaGameGameState.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AjinzzaGamePlayerController::AjinzzaGamePlayerController()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> GameEndWidgetBPClass(TEXT("/Game/JINZZA/UI/Widgets/WBP_GameEnd"));
	if (GameEndWidgetBPClass.Succeeded())
	{
		GameEndWidgetClass = GameEndWidgetBPClass.Class;
	}
}

void AjinzzaGamePlayerController::Client_ReceiveRoleAssignment_Implementation(EJinzzaPartyRole InRole, APlayerState* InRealOne)
{
	LocalRole = InRole;
	KnownRealOne = InRealOne;

	// TEMP placeholder role-reveal sound - swap for real SFX later.
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/BasketballHoop/correctanswer.correctanswer")))
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

void AjinzzaGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	TSubclassOf<UUserWidget> WidgetClass = GameEndWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UjinzzaGameEndWidget::StaticClass();
	}

	GameEndWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (GameEndWidget)
	{
		GameEndWidget->AddToViewport();
		GameEndWidget->SetIsFocusable(true);
		bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(GameEndWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}

	// TEMP placeholder in-round BGM (see UjinzzaMainMenuWidget/UjinzzaLobbyWidget for the same
	// pattern) - swap QuizGameBgm for a real match theme later.
	if (USoundBase* Bgm = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/Game/QuizGameBgm.QuizGameBgm")))
	{
		MusicComponent = UGameplayStatics::SpawnSound2D(this, Bgm, 1.f, 1.f, 0.f, nullptr, true, false);
	}

	if (AjinzzaGameGameState* JinzzaGameState = GetWorld() ? GetWorld()->GetGameState<AjinzzaGameGameState>() : nullptr)
	{
		PhaseChangedHandle = JinzzaGameState->OnPhaseChanged.AddUObject(this, &AjinzzaGamePlayerController::HandlePhaseChanged);
	}
}

void AjinzzaGamePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}

	if (AjinzzaGameGameState* JinzzaGameState = GetWorld() ? GetWorld()->GetGameState<AjinzzaGameGameState>() : nullptr)
	{
		JinzzaGameState->OnPhaseChanged.Remove(PhaseChangedHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AjinzzaGamePlayerController::HandlePhaseChanged(EJinzzaRoundPhase NewPhase)
{
	// TEMP placeholder phase-transition sound - swap for real SFX later.
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/Megaphone/PressButton.PressButton")))
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}
