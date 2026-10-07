// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaChatWidget.h"
#include "jinzzaPlayerController.h"
#include "jinzzaUIStyle.h"
#include "Components/Border.h"
#include "Components/EditableTextBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"

namespace
{
	constexpr int32 MaxChatLines = 50;
	constexpr double ChatVisibleSeconds = 6.0;
	constexpr double ChatFadeSeconds = 2.0;
	constexpr float ChatWidth = 520.f;
	constexpr float ChatHistoryHeight = 220.f;
}

void UjinzzaChatWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ChatBox"));
	Box->SetWidthOverride(ChatWidth);
	if (UOverlaySlot* BoxSlot = Root->AddChildToOverlay(Box))
	{
		BoxSlot->SetHorizontalAlignment(HAlign_Left);
		BoxSlot->SetVerticalAlignment(VAlign_Bottom);
		BoxSlot->SetPadding(FMargin(24.f, 0.f, 0.f, 24.f));
	}

	// Dark rounded backing - only drawn while typing (see NativeTick), the lines carry their own shadow.
	Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, 14.f));
	Panel->SetPadding(FMargin(12.f, 10.f));
	Box->AddChild(Panel);

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));
	Panel->SetContent(Stack);

	USizeBox* HistoryBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HistoryBox"));
	HistoryBox->SetMaxDesiredHeight(ChatHistoryHeight);
	Stack->AddChildToVerticalBox(HistoryBox);

	MessageList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("MessageList"));
	MessageList->SetScrollBarVisibility(ESlateVisibility::Collapsed);
	HistoryBox->AddChild(MessageList);

	InputBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("InputBox"));
	JinzzaUI::ApplyStickerStyle(InputBox);
	InputBox->SetHintText(FText::FromString(TEXT("Say something... (Enter to send, ESC to cancel)")));
	InputBox->SetVisibility(ESlateVisibility::Collapsed);
	JinzzaUI::AddSpaced(Stack, InputBox, 8.f);
}

void UjinzzaChatWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
	// Never takes clicks meant for the game or other HUD widgets - only the input line is interactive.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (InputBox)
	{
		InputBox->OnTextCommitted.AddDynamic(this, &UjinzzaChatWidget::HandleTextCommitted);
	}
}

void UjinzzaChatWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	float Opacity = 1.f;
	if (!bInputOpen)
	{
		const double Idle = FPlatformTime::Seconds() - LastActivityTime;
		Opacity = FMath::Clamp(1.f - static_cast<float>((Idle - ChatVisibleSeconds) / ChatFadeSeconds), 0.f, 1.f);
	}
	if (MessageList)
	{
		MessageList->SetRenderOpacity(Opacity);
	}
	if (Panel)
	{
		Panel->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, bInputOpen ? 0.55f : 0.f));
	}
}

void UjinzzaChatWidget::AddMessage(const FJinzzaChatMessage& Message)
{
	if (!MessageList || !WidgetTree)
	{
		return;
	}

	const FString Prefix = Message.bFromGhost ? TEXT("[Ghost] ") : TEXT("");
	UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Line->SetText(FText::FromString(FString::Printf(TEXT("%s%s: %s"), *Prefix, *Message.SenderName, *Message.Text)));
	Line->SetFont(JinzzaUI::BodyFont(18));
	Line->SetColorAndOpacity(FSlateColor(Message.bFromGhost ? JinzzaUI::Sticker_SubText : JinzzaUI::Sticker_White));
	Line->SetShadowOffset(FVector2D(1.5f, 1.5f));
	Line->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	Line->SetAutoWrapText(true);
	MessageList->AddChild(Line);

	while (MessageList->GetChildrenCount() > MaxChatLines)
	{
		MessageList->RemoveChildAt(0);
	}
	MessageList->ScrollToEnd();
	LastActivityTime = FPlatformTime::Seconds();
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
	if (MessageList)
	{
		MessageList->ScrollToEnd();
	}

	if (AjinzzaPlayerController* PC = Cast<AjinzzaPlayerController>(GetOwningPlayer()))
	{
		PC->EnterChatInputMode(InputBox);
	}
}

void UjinzzaChatWidget::CloseInput()
{
	if (!bInputOpen)
	{
		return;
	}
	bInputOpen = false;
	LastActivityTime = FPlatformTime::Seconds();
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

void UjinzzaChatWidget::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (!bInputOpen)
	{
		return;
	}

	if (CommitMethod == ETextCommit::OnEnter)
	{
		const FString Message = Text.ToString().TrimStartAndEnd();
		if (!Message.IsEmpty())
		{
			if (AjinzzaPlayerController* PC = Cast<AjinzzaPlayerController>(GetOwningPlayer()))
			{
				PC->Server_SendChatMessage(Message);
			}
		}
	}

	// Enter (sent or empty), ESC (OnCleared) and clicking away (OnUserMovedFocus) all close the line.
	CloseInput();
}
