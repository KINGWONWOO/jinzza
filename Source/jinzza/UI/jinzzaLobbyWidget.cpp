// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Styling/CoreStyle.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"

void UjinzzaLobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.04f, 0.6f));
	Panel->SetHorizontalAlignment(HAlign_Right);
	Panel->SetVerticalAlignment(VAlign_Top);
	Panel->SetPadding(FMargin(24.f));

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	PanelSlot->SetOffsets(FMargin(0.f));

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LobbyBox"));
	Panel->SetContent(Box);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LobbyTitle"));
	Title->SetText(FText::FromString(TEXT("LOBBY")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 32));
	Box->AddChildToVerticalBox(Title);

	PlayerCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayerCountText"));
	PlayerCountText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20));
	UVerticalBoxSlot* CountSlot = Box->AddChildToVerticalBox(PlayerCountText);
	CountSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 20.f));

	StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
	UTextBlock* StartLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartButtonLabel"));
	StartLabel->SetText(FText::FromString(TEXT("Start Match")));
	StartLabel->SetJustification(ETextJustify::Center);
	StartButton->AddChild(StartLabel);
	StartButton->OnClicked.AddDynamic(this, &UjinzzaLobbyWidget::OnStartMatchClicked);
	Box->AddChildToVerticalBox(StartButton);

	const APlayerController* PC = GetOwningPlayer();
	StartButton->SetVisibility(PC && PC->HasAuthority() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UjinzzaLobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (PlayerCountText)
	{
		int32 Count = 0;
		if (AGameStateBase* GameState = UGameplayStatics::GetGameState(this))
		{
			Count = GameState->PlayerArray.Num();
		}
		PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("Players connected: %d"), Count)));
	}
}

void UjinzzaLobbyWidget::OnStartMatchClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(TEXT("/Game/JINZZA/Level/Lvl_Game"));
		}
	}
}
