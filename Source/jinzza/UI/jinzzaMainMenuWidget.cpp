// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMainMenuWidget.h"
#include "jinzzaSettingsWidget.h"
#include "jinzzaUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

namespace
{
	enum EMenuPage : int32
	{
		Page_Buttons = 0,
		Page_Settings = 1,
	};

	constexpr float FadeInDuration = 0.35f;
}

void UjinzzaMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Switcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("MenuSwitcher"));
	WidgetTree->RootWidget = Switcher;

	// --- Page 0: main button list ---
	UBorder* FullscreenBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FullscreenBackground"));
	FullscreenBackground->SetBrushColor(JinzzaUI::Color_Background);
	FullscreenBackground->SetHorizontalAlignment(HAlign_Center);
	FullscreenBackground->SetVerticalAlignment(VAlign_Center);
	Switcher->AddChild(FullscreenBackground);

	UBorder* Panel = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("ButtonsPanel"));
	Panel->SetPadding(FMargin(56.f, 48.f));
	FullscreenBackground->SetContent(Panel);
	ButtonsPageRoot = Panel;

	UVerticalBox* MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuBox"));
	Panel->SetContent(MenuBox);

	UTextBlock* Title = JinzzaUI::MakeTitleText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("진짜를 찾아라")), 52);
	UVerticalBoxSlot* TitleSlot = MenuBox->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

	UVerticalBoxSlot* DividerSlot = MenuBox->AddChildToVerticalBox(JinzzaUI::MakeDivider(WidgetTree, TEXT("TitleDivider")));
	DividerSlot->SetHorizontalAlignment(HAlign_Center);
	DividerSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	UTextBlock* Tagline = JinzzaUI::MakeBodyText(WidgetTree, TEXT("Tagline"), FText::FromString(TEXT("Find the Real One")), true);
	UVerticalBoxSlot* TaglineSlot = MenuBox->AddChildToVerticalBox(Tagline);
	TaglineSlot->SetHorizontalAlignment(HAlign_Center);
	TaglineSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::GetEmpty());
	StatusText->SetFont(JinzzaUI::BodyFont(16));
	StatusText->SetColorAndOpacity(FSlateColor(JinzzaUI::Color_Accent));
	StatusText->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* StatusSlot = MenuBox->AddChildToVerticalBox(StatusText);
	StatusSlot->SetHorizontalAlignment(HAlign_Center);
	StatusSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));

	UButton* HostButton = JinzzaUI::MakePrimaryButton(WidgetTree, TEXT("HostButton"), FText::FromString(TEXT("HOST GAME")));
	HostButton->OnClicked.AddDynamic(this, &UjinzzaMainMenuWidget::OnHostClicked);
	UVerticalBoxSlot* HostSlot = MenuBox->AddChildToVerticalBox(HostButton);
	HostSlot->SetHorizontalAlignment(HAlign_Fill);
	HostSlot->SetPadding(FMargin(0.f, 8.f));

	UTextBlock* InviteHint = JinzzaUI::MakeBodyText(WidgetTree, TEXT("InviteHint"),
		FText::FromString(TEXT("Friends join by accepting your Steam invite from the Lobby.")), true);
	InviteHint->SetJustification(ETextJustify::Center);
	InviteHint->SetAutoWrapText(true);
	UVerticalBoxSlot* InviteHintSlot = MenuBox->AddChildToVerticalBox(InviteHint);
	InviteHintSlot->SetHorizontalAlignment(HAlign_Center);
	InviteHintSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));

	UButton* SettingsButton = JinzzaUI::MakeSecondaryButton(WidgetTree, TEXT("SettingsButton"), FText::FromString(TEXT("Settings")));
	SettingsButton->OnClicked.AddDynamic(this, &UjinzzaMainMenuWidget::OnSettingsClicked);
	UVerticalBoxSlot* SettingsSlot = MenuBox->AddChildToVerticalBox(SettingsButton);
	SettingsSlot->SetHorizontalAlignment(HAlign_Fill);
	SettingsSlot->SetPadding(FMargin(0.f, 8.f));

	UButton* QuitButton = JinzzaUI::MakeWarningButton(WidgetTree, TEXT("QuitButton"), FText::FromString(TEXT("Quit")));
	QuitButton->OnClicked.AddDynamic(this, &UjinzzaMainMenuWidget::OnQuitClicked);
	UVerticalBoxSlot* QuitSlot = MenuBox->AddChildToVerticalBox(QuitButton);
	QuitSlot->SetHorizontalAlignment(HAlign_Fill);
	QuitSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

	// --- Page 1: Settings popup, a fully independent embedded UserWidget ---
	SettingsWidget = CreateWidget<UjinzzaSettingsWidget>(this, UjinzzaSettingsWidget::StaticClass());
	Switcher->AddChild(SettingsWidget);
	SettingsWidget->OnBackRequested.AddUObject(this, &UjinzzaMainMenuWidget::ShowButtonsPage);

	Switcher->SetActiveWidgetIndex(Page_Buttons);

	if (UjinzzaGameInstance* GI = GetJinzzaGameInstance())
	{
		SessionStatusHandle = GI->OnSessionStatusChanged.AddUObject(this, &UjinzzaMainMenuWidget::HandleSessionStatusChanged);
	}

	if (ButtonsPageRoot)
	{
		ButtonsPageRoot->SetRenderOpacity(0.f);
	}

	if (USoundBase* Bgm = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/MainMenu/MainMenuBgm_Cue.MainMenuBgm_Cue")))
	{
		MusicComponent = UGameplayStatics::SpawnSound2D(this, Bgm, 1.f, 1.f, 0.f, nullptr, true, false);
	}
}

void UjinzzaMainMenuWidget::NativeDestruct()
{
	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}

	if (UjinzzaGameInstance* GI = GetJinzzaGameInstance())
	{
		GI->OnSessionStatusChanged.Remove(SessionStatusHandle);
	}

	Super::NativeDestruct();
}

void UjinzzaMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (ButtonsPageRoot && FadeInElapsed < FadeInDuration && Switcher && Switcher->GetActiveWidgetIndex() == Page_Buttons)
	{
		FadeInElapsed = FMath::Min(FadeInElapsed + InDeltaTime, FadeInDuration);
		ButtonsPageRoot->SetRenderOpacity(FMath::Clamp(FadeInElapsed / FadeInDuration, 0.f, 1.f));
	}
}

UjinzzaGameInstance* UjinzzaMainMenuWidget::GetJinzzaGameInstance() const
{
	return Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this));
}

void UjinzzaMainMenuWidget::ShowButtonsPage()
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(Page_Buttons);
	}
}

void UjinzzaMainMenuWidget::OnHostClicked()
{
	if (UjinzzaGameInstance* GI = GetJinzzaGameInstance())
	{
		GI->HostSession(FJinzzaMatchSettings());
	}
}

void UjinzzaMainMenuWidget::OnSettingsClicked()
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(Page_Settings);
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
