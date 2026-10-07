// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaPauseMenuWidget.h"
#include "jinzzaSettingsWidget.h"
#include "jinzzaUIStyle.h"
#include "jinzzaGameInstance.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"

namespace
{
	enum EPauseMenuPage : int32
	{
		PausePage_Buttons = 0,
		PausePage_Settings = 1,
	};
}

void UjinzzaPauseMenuWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	// Transparent root - only the centered panel covers the game.
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	Switcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Switcher"));
	if (UOverlaySlot* SwitcherSlot = Root->AddChildToOverlay(Switcher))
	{
		SwitcherSlot->SetHorizontalAlignment(HAlign_Center);
		SwitcherSlot->SetVerticalAlignment(VAlign_Center);
	}

	// Page 0: small black sticker panel with a column of sticker pills.
	UBorder* PanelFace = nullptr;
	UOverlay* Panel = JinzzaUI::MakeStickerPanel(WidgetTree, TEXT("Panel"), PanelFace);
	Switcher->AddChild(Panel);
	if (UWidgetSwitcherSlot* PanelSlot = Cast<UWidgetSwitcherSlot>(Panel->Slot))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* PanelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelBox"));
	PanelBox->SetWidthOverride(380.f);
	PanelFace->SetContent(PanelBox);

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));
	PanelBox->AddChild(Stack);

	if (UVerticalBoxSlot* TitleSlot = Stack->AddChildToVerticalBox(JinzzaUI::MakeStickerHeading(WidgetTree, TEXT("Heading"), FText::FromString(TEXT("Menu")), 40)))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// Hover colors follow the main menu: Settings sky blue, quitting coral, Cancel yellow.
	SettingsButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("SettingsButton"), FText::FromString(TEXT("Settings")), JinzzaUI::Sticker_Sky, 24.f);
	JinzzaUI::AddSpaced(Stack, SettingsButton, 22.f);

	QuitGameButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("QuitGameButton"), FText::FromString(TEXT("Quit Game")), JinzzaUI::Sticker_Coral, 24.f);
	JinzzaUI::AddSpaced(Stack, QuitGameButton, 12.f);

	ExitToDesktopButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("ExitToDesktopButton"), FText::FromString(TEXT("Exit to Desktop")), JinzzaUI::Sticker_Coral, 24.f);
	JinzzaUI::AddSpaced(Stack, ExitToDesktopButton, 12.f);

	CancelButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("CancelButton"), FText::FromString(TEXT("Cancel")), JinzzaUI::Sticker_Yellow, 24.f, true);
	JinzzaUI::AddSpaced(Stack, CancelButton, 22.f);

	// Page 1: the main menu's settings screen, shrunk to a window.
	SettingsWidget = WidgetTree->ConstructWidget<UjinzzaSettingsWidget>(UjinzzaSettingsWidget::StaticClass(), TEXT("SettingsWidget"));
	Switcher->AddChild(SettingsWidget);
	if (UWidgetSwitcherSlot* SettingsSlot = Cast<UWidgetSwitcherSlot>(SettingsWidget->Slot))
	{
		SettingsSlot->SetHorizontalAlignment(HAlign_Center);
		SettingsSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UjinzzaPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
	SetIsFocusable(true);

	if (SettingsButton) SettingsButton->OnClicked.AddDynamic(this, &UjinzzaPauseMenuWidget::OnSettingsClicked);
	if (QuitGameButton) QuitGameButton->OnClicked.AddDynamic(this, &UjinzzaPauseMenuWidget::OnQuitGameClicked);
	if (ExitToDesktopButton) ExitToDesktopButton->OnClicked.AddDynamic(this, &UjinzzaPauseMenuWidget::OnExitToDesktopClicked);
	if (CancelButton) CancelButton->OnClicked.AddDynamic(this, &UjinzzaPauseMenuWidget::OnCancelClicked);

	if (SettingsWidget)
	{
		SettingsWidget->SetCompactLayout();
		SettingsWidget->OnBackRequested.AddUObject(this, &UjinzzaPauseMenuWidget::ShowButtonsPage);
	}

	ShowButtonsPage();
}

FReply UjinzzaPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Reaches here only if the focused child didn't handle it - the settings screen consumes the key
	// itself while waiting for a rebind, so binding a key to ESC still works.
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Right)
	{
		if (Switcher && Switcher->GetActiveWidgetIndex() == PausePage_Settings)
		{
			ShowButtonsPage();
		}
		else
		{
			OnCloseRequested.Broadcast();
		}
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UjinzzaPauseMenuWidget::ShowButtonsPage()
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(PausePage_Buttons);
	}
	// Keep keyboard focus inside this widget so ESC keeps reaching NativeOnKeyDown.
	SetFocus();
}

void UjinzzaPauseMenuWidget::OnSettingsClicked()
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(PausePage_Settings);
	}
}

void UjinzzaPauseMenuWidget::OnQuitGameClicked()
{
	if (UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->LeaveToTitle();
	}
}

void UjinzzaPauseMenuWidget::OnExitToDesktopClicked()
{
	if (UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->DestroySession();
	}
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UjinzzaPauseMenuWidget::OnCancelClicked()
{
	OnCloseRequested.Broadcast();
}
