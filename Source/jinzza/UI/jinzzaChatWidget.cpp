// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaChatWidget.h"
#include "jinzzaChatTypes.h"
#include "jinzzaPlayerController.h"
#include "jinzzaUIStyle.h"
#include "Components/EditableTextBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"

void UjinzzaChatWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InputBoxSize"));
	Box->SetWidthOverride(520.f);
	if (UOverlaySlot* BoxSlot = Root->AddChildToOverlay(Box))
	{
		BoxSlot->SetHorizontalAlignment(HAlign_Left);
		BoxSlot->SetVerticalAlignment(VAlign_Bottom);
		BoxSlot->SetPadding(FMargin(24.f, 0.f, 0.f, 24.f));
	}

	InputBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("InputBox"));
	JinzzaUI::ApplyStickerStyle(InputBox);
	InputBox->SetHintText(FText::FromString(TEXT("Write on your board... (Enter to show, ESC to cancel)")));
	InputBox->SetVisibility(ESlateVisibility::Collapsed);
	Box->AddChild(InputBox);
}

void UjinzzaChatWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
	// Never takes clicks meant for the game or other HUD widgets - only the input line is interactive.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (InputBox)
	{
		InputBox->OnTextChanged.AddDynamic(this, &UjinzzaChatWidget::HandleTextChanged);
		InputBox->OnTextCommitted.AddDynamic(this, &UjinzzaChatWidget::HandleTextCommitted);
	}
}

void UjinzzaChatWidget::OpenInput()
{
	if (bInputOpen || !InputBox)
	{
		return;
	}
	bInputOpen = true;
	InputBox->SetText(FText::GetEmpty());
	InputBox->SetVisibility(ESlateVisibility::Visible);

	if (AjinzzaPlayerController* PC = Cast<AjinzzaPlayerController>(GetOwningPlayer()))
	{
		PC->EnterChatInputMode(InputBox);
		PC->Server_SetChatWriting(true);
	}
}

void UjinzzaChatWidget::CloseInput()
{
	if (!bInputOpen)
	{
		return;
	}
	bInputOpen = false;
	if (InputBox)
	{
		InputBox->SetText(FText::GetEmpty());
		InputBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (AjinzzaPlayerController* PC = Cast<AjinzzaPlayerController>(GetOwningPlayer()))
	{
		PC->ExitChatInputMode();
	}
}

void UjinzzaChatWidget::CancelInput()
{
	if (!bInputOpen)
	{
		return;
	}
	if (AjinzzaPlayerController* PC = Cast<AjinzzaPlayerController>(GetOwningPlayer()))
	{
		PC->Server_SetChatWriting(false);
	}
	CloseInput();
}

void UjinzzaChatWidget::HandleTextChanged(const FText& Text)
{
	if (!bInputOpen)
	{
		return;
	}

	// Keep it to what the server will accept, so the preview matches what others will see.
	FString Current = Text.ToString();
	if (Current.Len() > JinzzaChat::MaxMessageLength)
	{
		Current.LeftInline(JinzzaChat::MaxMessageLength);
		InputBox->SetText(FText::FromString(Current));
	}

	if (AjinzzaPlayerController* PC = Cast<AjinzzaPlayerController>(GetOwningPlayer()))
	{
		PC->UpdateChatPreview(Current);
	}
}

void UjinzzaChatWidget::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (!bInputOpen)
	{
		return;
	}

	AjinzzaPlayerController* PC = Cast<AjinzzaPlayerController>(GetOwningPlayer());
	const FString Message = Text.ToString().TrimStartAndEnd();
	if (PC)
	{
		if (CommitMethod == ETextCommit::OnEnter && !Message.IsEmpty())
		{
			PC->Server_SendChatMessage(Message);
		}
		else
		{
			// ESC (OnCleared), clicking away (OnUserMovedFocus) or an empty Enter: lower the board.
			PC->Server_SetChatWriting(false);
		}
	}

	CloseInput();
}
