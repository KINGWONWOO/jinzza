// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMainMenuWidget.h"
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
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"

namespace
{
	UButton* MakeMenuButton(UWidgetTree* Tree, FName Name, const FText& Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);

		UTextBlock* ButtonText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Label")));
		ButtonText->SetText(Label);
		ButtonText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 24));
		ButtonText->SetJustification(ETextJustify::Center);

		Button->AddChild(ButtonText);
		return Button;
	}
}

void UjinzzaMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.04f, 1.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);

	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackgroundSlot->SetOffsets(FMargin(0.f));

	UVerticalBox* MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuBox"));
	Background->SetContent(MenuBox);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	Title->SetText(FText::FromString(TEXT("진짜를 찾아라")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 56));
	Title->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* TitleSlot = MenuBox->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 40.f));

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::GetEmpty());
	StatusText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 18));
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.8f, 0.3f, 1.f)));
	StatusText->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* StatusSlot = MenuBox->AddChildToVerticalBox(StatusText);
	StatusSlot->SetHorizontalAlignment(HAlign_Center);
	StatusSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 30.f));

	UButton* HostButton = MakeMenuButton(WidgetTree, TEXT("HostButton"), FText::FromString(TEXT("Host Game")));
	HostButton->OnClicked.AddDynamic(this, &UjinzzaMainMenuWidget::OnHostClicked);
	UVerticalBoxSlot* HostSlot = MenuBox->AddChildToVerticalBox(HostButton);
	HostSlot->SetHorizontalAlignment(HAlign_Fill);
	HostSlot->SetPadding(FMargin(0.f, 8.f));

	UButton* JoinButton = MakeMenuButton(WidgetTree, TEXT("JoinButton"), FText::FromString(TEXT("Join Game")));
	JoinButton->OnClicked.AddDynamic(this, &UjinzzaMainMenuWidget::OnJoinClicked);
	UVerticalBoxSlot* JoinSlot = MenuBox->AddChildToVerticalBox(JoinButton);
	JoinSlot->SetHorizontalAlignment(HAlign_Fill);
	JoinSlot->SetPadding(FMargin(0.f, 8.f));

	UButton* SettingsButton = MakeMenuButton(WidgetTree, TEXT("SettingsButton"), FText::FromString(TEXT("Settings")));
	SettingsButton->OnClicked.AddDynamic(this, &UjinzzaMainMenuWidget::OnSettingsClicked);
	UVerticalBoxSlot* SettingsSlot = MenuBox->AddChildToVerticalBox(SettingsButton);
	SettingsSlot->SetHorizontalAlignment(HAlign_Fill);
	SettingsSlot->SetPadding(FMargin(0.f, 8.f));

	UButton* QuitButton = MakeMenuButton(WidgetTree, TEXT("QuitButton"), FText::FromString(TEXT("Quit")));
	QuitButton->OnClicked.AddDynamic(this, &UjinzzaMainMenuWidget::OnQuitClicked);
	UVerticalBoxSlot* QuitSlot = MenuBox->AddChildToVerticalBox(QuitButton);
	QuitSlot->SetHorizontalAlignment(HAlign_Fill);
	QuitSlot->SetPadding(FMargin(0.f, 8.f));

	if (UjinzzaGameInstance* GI = GetJinzzaGameInstance())
	{
		SessionStatusHandle = GI->OnSessionStatusChanged.AddUObject(this, &UjinzzaMainMenuWidget::HandleSessionStatusChanged);
	}
}

void UjinzzaMainMenuWidget::NativeDestruct()
{
	if (UjinzzaGameInstance* GI = GetJinzzaGameInstance())
	{
		GI->OnSessionStatusChanged.Remove(SessionStatusHandle);
	}

	Super::NativeDestruct();
}

UjinzzaGameInstance* UjinzzaMainMenuWidget::GetJinzzaGameInstance() const
{
	return Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this));
}

void UjinzzaMainMenuWidget::OnHostClicked()
{
	if (UjinzzaGameInstance* GI = GetJinzzaGameInstance())
	{
		GI->HostSession();
	}
}

void UjinzzaMainMenuWidget::OnJoinClicked()
{
	if (UjinzzaGameInstance* GI = GetJinzzaGameInstance())
	{
		GI->QuickJoinSession();
	}
}

void UjinzzaMainMenuWidget::OnSettingsClicked()
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("Settings coming soon")));
	}
}

void UjinzzaMainMenuWidget::OnQuitClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
}

void UjinzzaMainMenuWidget::HandleSessionStatusChanged(EJinzzaSessionStatus Status, const FString& Message)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
	}
}
