// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGamePlayerController.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaGameEndWidget.h"
#include "jinzzaGameGameState.h"
#include "jinzzaGameGameMode.h"
#include "jinzzaPartyPlayerState.h"
#include "jinzzaSpeakTurnWidget.h"
#include "jinzzaVoteWidget.h"
#include "Camera/CameraActor.h"
#include "jinzzaLoadingScreenSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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

void AjinzzaGamePlayerController::Client_ReceiveTurnMessage_Implementation(APlayerState* Speaker, const FString& Text)
{
	if (SpeakTurnWidget)
	{
		SpeakTurnWidget->AddBubble(AjinzzaPartyPlayerState::GetDisplayNameFor(Speaker), Text, Speaker && Speaker == PlayerState);
	}
}

void AjinzzaGamePlayerController::Server_CastVote_Implementation(APlayerState* Candidate)
{
	if (AjinzzaGameGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AjinzzaGameGameMode>() : nullptr)
	{
		GameMode->HandleVote(this, Candidate);
	}
}

bool AjinzzaGamePlayerController::IsVoiceBlocked() const
{
	const AjinzzaGameGameState* MatchState = GetWorld() ? GetWorld()->GetGameState<AjinzzaGameGameState>() : nullptr;
	return MatchState && MatchState->IsSpeakTurnActive() && !MatchState->IsTurnSpeaker(PlayerState);
}

bool AjinzzaGamePlayerController::CanUseChat() const
{
	return !IsVoiceBlocked();
}

bool AjinzzaGamePlayerController::ShouldUseChatBoard() const
{
	const AjinzzaGameGameState* MatchState = GetWorld() ? GetWorld()->GetGameState<AjinzzaGameGameState>() : nullptr;
	return !MatchState || !MatchState->IsSpeakTurnActive();
}

bool AjinzzaGamePlayerController::RouteChatMessage(const FString& Clean)
{
	const AjinzzaGameGameState* MatchState = GetWorld() ? GetWorld()->GetGameState<AjinzzaGameGameState>() : nullptr;
	if (!MatchState || !MatchState->IsSpeakTurnActive())
	{
		return false;
	}

	// During a turn only the speaker's lines go anywhere - as speech bubbles, to everyone.
	if (MatchState->IsTurnSpeaker(PlayerState))
	{
		if (AjinzzaGameGameMode* GameMode = GetWorld()->GetAuthGameMode<AjinzzaGameGameMode>())
		{
			GameMode->BroadcastTurnMessage(PlayerState, Clean);
		}
	}
	return true;
}

void AjinzzaGamePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!IsLocalController())
	{
		return;
	}

	// Normally the loading screen reports us loaded once it's done; with no loading screen (e.g. PIE started
	// straight in Lvl_Game) do it as soon as we have our character, or the round would only start on timeout.
	if (!bLoadReported && GetPawn() && !IsLoadingScreenUp())
	{
		ReportLoadComplete();
	}

	UpdateSpeakTurn();
	if (SpeakTurnWidget)
	{
		SpeakTurnWidget->Refresh();
	}
	if (VoteWidget)
	{
		VoteWidget->Refresh();
	}
}

void AjinzzaGamePlayerController::UpdateSpeakTurn()
{
	const AjinzzaGameGameState* MatchState = GetWorld() ? GetWorld()->GetGameState<AjinzzaGameGameState>() : nullptr;
	if (!MatchState)
	{
		return;
	}

	const FJinzzaSpeakTurn& Turn = MatchState->GetSpeakTurn();
	const bool bActive = MatchState->IsSpeakTurnActive();
	const bool bSpeaker = MatchState->IsTurnSpeaker(PlayerState);

	// Nobody moves during a turn (the speaker stays on the spotlight). Re-asserted every frame: a possession
	// (ClientRestart) resets the engine's ignore-input counters.
	if (bActive != bTurnMovementLocked)
	{
		bTurnMovementLocked = bActive;
		SetIgnoreMoveInput(bActive);
	}
	else if (bActive && !IsMoveInputIgnored())
	{
		SetIgnoreMoveInput(true);
	}

	// Also re-applied if we turn out to be the speaker after all (speaker reference resolved late).
	if (Turn.Serial == AppliedTurnSerial && bSpeaker == bAppliedAsSpeaker)
	{
		return;
	}
	AppliedTurnSerial = Turn.Serial;
	bAppliedAsSpeaker = bSpeaker;

	if (bActive && !bSpeaker)
	{
		// Mid-sentence chat is cancelled - only the speaker may write.
		CancelChatInput();

		if (!TurnCamera)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags |= RF_Transient;
			TurnCamera = GetWorld()->SpawnActor<ACameraActor>(Turn.CameraLocation, Turn.CameraRotation, Params);
		}
		if (TurnCamera)
		{
			TurnCamera->SetActorLocationAndRotation(Turn.CameraLocation, Turn.CameraRotation);
			SetViewTargetWithBlend(TurnCamera, bWatchingTurnCamera ? 0.f : 0.4f, VTBlend_EaseInOut, 2.f);
			bWatchingTurnCamera = true;
		}
	}
	else if (bWatchingTurnCamera)
	{
		// Turn over, or it's our own turn: back to our own eyes.
		bWatchingTurnCamera = false;
		if (APawn* MyPawn = GetPawn())
		{
			SetViewTargetWithBlend(MyPawn, 0.4f, VTBlend_EaseInOut, 2.f);
		}
	}
}

void AjinzzaGamePlayerController::ReportLoadComplete()
{
	if (!bLoadReported)
	{
		bLoadReported = true;
		Server_ReportLoaded();
	}
}

void AjinzzaGamePlayerController::Server_ReportLoaded_Implementation()
{
	if (AjinzzaGameGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AjinzzaGameGameMode>() : nullptr)
	{
		GameMode->NotifyPlayerLoaded(this);
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
		RestoreGameplayInputMode();
	}

	// Turn banner + speech bubbles (everyone) and the Judge's ballot (shows itself only for the Judge).
	SpeakTurnWidget = CreateWidget<UjinzzaSpeakTurnWidget>(this, UjinzzaSpeakTurnWidget::StaticClass());
	if (SpeakTurnWidget)
	{
		SpeakTurnWidget->AddToViewport(30);
	}
	VoteWidget = CreateWidget<UjinzzaVoteWidget>(this, UjinzzaVoteWidget::StaticClass());
	if (VoteWidget)
	{
		VoteWidget->AddToViewport(35);
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

void AjinzzaGamePlayerController::RestoreGameplayInputMode()
{
	if (!GameEndWidget)
	{
		Super::RestoreGameplayInputMode();
		return;
	}

	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(GameEndWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AjinzzaGamePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}

	if (TurnCamera)
	{
		TurnCamera->Destroy();
		TurnCamera = nullptr;
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
