// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaChatBoardComponent.h"
#include "jinzzaChatBoardWidget.h"
#include "jinzzaPartyPlayerState.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace
{
	constexpr float RaiseSeconds = 0.2f;
	constexpr float FlipSeconds = 0.35f;
	/** How far below its held position (cm) the board starts/ends when raised/lowered. */
	constexpr float LoweredDrop = 35.f;

	float DisplaySecondsFor(const FString& Text)
	{
		return FMath::Clamp(3.f + Text.Len() * 0.06f, 4.f, 10.f);
	}
}

UjinzzaChatBoardComponent::UjinzzaChatBoardComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UjinzzaChatBoardComponent::Bind(USceneComponent* InPivot, UWidgetComponent* InTextWidget)
{
	Pivot = InPivot;
	TextWidget = InTextWidget;
}

void UjinzzaChatBoardComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Pivot)
	{
		PivotRestLocation = Pivot->GetRelativeLocation();
		Pivot->SetVisibility(false, true);
	}
}

void UjinzzaChatBoardComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UjinzzaChatBoardComponent, Board);
}

// --- Server ---------------------------------------------------------------------------------------

void UjinzzaChatBoardComponent::SetBoard(EJinzzaChatBoardState NewState, const FString& NewText)
{
	Board.State = NewState;
	Board.Text = NewText;
	// The listen server's own copy never gets a RepNotify.
	OnRep_Board();
}

void UjinzzaChatBoardComponent::ServerStartWriting()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(HideTimerHandle);
	SetBoard(EJinzzaChatBoardState::Writing, FString());
}

void UjinzzaChatBoardComponent::ServerCancelWriting()
{
	if (GetOwner() && GetOwner()->HasAuthority() && Board.State == EJinzzaChatBoardState::Writing)
	{
		SetBoard(EJinzzaChatBoardState::Hidden, FString());
	}
}

void UjinzzaChatBoardComponent::ServerReveal(const FString& Text)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	SetBoard(EJinzzaChatBoardState::Showing, Text);
	GetWorld()->GetTimerManager().SetTimer(HideTimerHandle, this, &UjinzzaChatBoardComponent::ServerHide, DisplaySecondsFor(Text), false);
}

void UjinzzaChatBoardComponent::ServerHide()
{
	if (Board.State == EJinzzaChatBoardState::Showing)
	{
		SetBoard(EJinzzaChatBoardState::Hidden, FString());
	}
}

// --- Local visuals --------------------------------------------------------------------------------

void UjinzzaChatBoardComponent::OnRep_Board()
{
	if (Board.State == EJinzzaChatBoardState::Writing)
	{
		// A fresh board always starts written side toward the owner, even if it was still showing.
		Yaw = 180.f;
	}
	if (Board.State != EJinzzaChatBoardState::Writing)
	{
		LocalPreviewText.Reset();
	}
	RefreshText();
}

void UjinzzaChatBoardComponent::SetLocalPreviewText(const FString& Text)
{
	LocalPreviewText = Text;
	RefreshText();
}

void UjinzzaChatBoardComponent::RefreshText()
{
	UjinzzaChatBoardWidget* Widget = TextWidget ? Cast<UjinzzaChatBoardWidget>(TextWidget->GetUserWidgetObject()) : nullptr;
	if (!Widget)
	{
		return;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const bool bOwnerView = OwnerPawn && OwnerPawn->IsLocallyControlled();
	if (Board.State == EJinzzaChatBoardState::Showing)
	{
		Widget->SetBoardText(Board.Text);
	}
	else if (Board.State == EJinzzaChatBoardState::Writing && bOwnerView)
	{
		Widget->SetBoardText(LocalPreviewText);
	}
	else
	{
		Widget->SetBoardText(FString());
	}
}

bool UjinzzaChatBoardComponent::CanLocalViewerSee() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AjinzzaPartyPlayerState* OwnerState = OwnerPawn ? OwnerPawn->GetPlayerState<AjinzzaPartyPlayerState>() : nullptr;
	if (!OwnerState || !OwnerState->IsGhost() || OwnerPawn->IsLocallyControlled())
	{
		return true;
	}

	const APlayerController* LocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr;
	const AjinzzaPartyPlayerState* ViewerState = LocalPC ? LocalPC->GetPlayerState<AjinzzaPartyPlayerState>() : nullptr;
	return ViewerState && ViewerState->IsGhost();
}

void UjinzzaChatBoardComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Pivot)
	{
		return;
	}

	const bool bWantUp = Board.State != EJinzzaChatBoardState::Hidden;
	Raise = FMath::FInterpConstantTo(Raise, bWantUp ? 1.f : 0.f, DeltaTime, 1.f / RaiseSeconds);

	// Flip only once it's up: Writing holds 180 (toward the owner), Showing turns to 0 (toward everyone).
	const float TargetYaw = Board.State == EJinzzaChatBoardState::Showing ? 0.f : 180.f;
	if (Raise > 0.95f || Board.State == EJinzzaChatBoardState::Hidden)
	{
		Yaw = FMath::FInterpConstantTo(Yaw, TargetYaw, DeltaTime, 180.f / FlipSeconds);
	}

	const bool bVisible = Raise > KINDA_SMALL_NUMBER && CanLocalViewerSee();
	if (Pivot->IsVisible() != bVisible)
	{
		Pivot->SetVisibility(bVisible, true);
	}
	if (bVisible)
	{
		const float Eased = FMath::InterpEaseOut(0.f, 1.f, Raise, 2.f);
		Pivot->SetRelativeLocationAndRotation(PivotRestLocation - FVector(0.f, 0.f, LoweredDrop * (1.f - Eased)), FRotator(0.f, Yaw, 0.f));
	}
}
