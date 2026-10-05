// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaBoomboxWidget.h"
#include "jinzzaUIStyle.h"
#include "jinzzaBoomboxProp.h"
#include "jinzzaCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

void UJinzzaBoomboxRowHandler::HandleClicked()
{
	if (UjinzzaBoomboxWidget* Owner = OwnerWidget.Get())
	{
		Owner->OnTrackClicked(TrackIndex);
	}
}

void UjinzzaBoomboxWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Dimmed full-screen backdrop; NativeOnMouseButtonDown swallows clicks that land on it.
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.35f));
	if (UOverlaySlot* BackdropSlot = Root->AddChildToOverlay(Backdrop))
	{
		BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
		BackdropSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// Sticker-style panel, matching the main menu / T_Logo.
	UBorder* PanelFace = nullptr;
	UOverlay* Panel = JinzzaUI::MakeStickerPanel(WidgetTree, TEXT("Panel"), PanelFace);
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* PanelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelBox"));
	PanelBox->SetWidthOverride(760.f);
	PanelFace->SetContent(PanelBox);

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));
	PanelBox->AddChild(Stack);

	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeStickerHeading(WidgetTree, TEXT("Title"), FText::FromString(TEXT("Boombox")), 48), 0.f);

	StatusText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("StatusText"), FText::FromString(TEXT("Nothing playing")), 22, false);
	JinzzaUI::AddSpaced(Stack, StatusText, 12.f);

	JinzzaUI::AddStickerSection(WidgetTree, Stack, TEXT("SongsSection"), FText::FromString(TEXT("Songs")));
	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeStickerText(WidgetTree, TEXT("SongsHint"), FText::FromString(TEXT("Click a song to play it. Click it again to pause.")), 18, true), 2.f);

	UScrollBox* TrackScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("TrackScroll"));
	USizeBox* TrackHeightBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TrackHeightBox"));
	TrackHeightBox->SetHeightOverride(260.f);
	TrackHeightBox->AddChild(TrackScroll);
	JinzzaUI::AddSpaced(Stack, TrackHeightBox, 8.f);

	TrackListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TrackListBox"));
	TrackScroll->AddChild(TrackListBox);

	JinzzaUI::AddStickerSection(WidgetTree, Stack, TEXT("LinkSection"), FText::FromString(TEXT("Play a Link")));

	UHorizontalBox* LinkRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LinkRow"));
	JinzzaUI::AddSpaced(Stack, LinkRow, 8.f);

	LinkTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("LinkTextBox"));
	LinkTextBox->SetHintText(FText::FromString(TEXT("https://example.com/song.mp3")));
	JinzzaUI::ApplyStickerStyle(LinkTextBox);
	if (UHorizontalBoxSlot* LinkSlot = LinkRow->AddChildToHorizontalBox(LinkTextBox))
	{
		LinkSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LinkSlot->SetVerticalAlignment(VAlign_Center);
		LinkSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}

	PlayLinkButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("PlayLinkButton"), FText::FromString(TEXT("Play")), JinzzaUI::Sticker_Pink, 22.f, true);
	LinkRow->AddChildToHorizontalBox(PlayLinkButton);

	LinkMessageText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("LinkMessage"),
		FText::FromString(TEXT("Needs a direct link to an audio file (.mp3, .m4a, .wav ...). YouTube / Spotify page links can't be played.")), 17, true);
	LinkMessageText->SetAutoWrapText(true);
	JinzzaUI::AddSpaced(Stack, LinkMessageText, 6.f);

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
	if (UVerticalBoxSlot* ButtonRowSlot = Stack->AddChildToVerticalBox(ButtonRow))
	{
		ButtonRowSlot->SetHorizontalAlignment(HAlign_Right);
		ButtonRowSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}

	CloseButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("CloseButton"), FText::FromString(TEXT("Close")), JinzzaUI::Sticker_Sky, 26.f);
	if (UHorizontalBoxSlot* CloseSlot = ButtonRow->AddChildToHorizontalBox(CloseButton))
	{
		CloseSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}

	PlayPauseButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("PlayPauseButton"), FText::FromString(TEXT("Play")), JinzzaUI::Sticker_Yellow, 28.f, true);
	PlayPauseLabel = Cast<UTextBlock>(PlayPauseButton->GetContent());
	ButtonRow->AddChildToHorizontalBox(PlayPauseButton);
}

void UjinzzaBoomboxWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();

	if (PlayLinkButton)
	{
		PlayLinkButton->OnClicked.AddDynamic(this, &UjinzzaBoomboxWidget::OnPlayLinkClicked);
	}
	if (LinkTextBox)
	{
		LinkTextBox->OnTextCommitted.AddDynamic(this, &UjinzzaBoomboxWidget::OnLinkCommitted);
	}
	if (PlayPauseButton)
	{
		PlayPauseButton->OnClicked.AddDynamic(this, &UjinzzaBoomboxWidget::OnPlayPauseClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UjinzzaBoomboxWidget::OnCloseClicked);
	}

	SetIsFocusable(true);
}

void UjinzzaBoomboxWidget::NativeDestruct()
{
	if (AjinzzaBoomboxProp* Prop = Boombox.Get())
	{
		Prop->OnMusicStateChanged.Remove(MusicStateChangedHandle);
	}
	MusicStateChangedHandle.Reset();

	Super::NativeDestruct();
}

void UjinzzaBoomboxWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bRefreshPending)
	{
		bRefreshPending = false;
		RefreshFromBoombox();
	}
}

void UjinzzaBoomboxWidget::SetBoombox(AjinzzaBoomboxProp* InBoombox)
{
	if (AjinzzaBoomboxProp* Old = Boombox.Get())
	{
		Old->OnMusicStateChanged.Remove(MusicStateChangedHandle);
	}

	Boombox = InBoombox;
	if (InBoombox)
	{
		MusicStateChangedHandle = InBoombox->OnMusicStateChanged.AddUObject(this, &UjinzzaBoomboxWidget::HandleMusicStateChanged);
	}
	bRefreshPending = true;
}

void UjinzzaBoomboxWidget::HandleMusicStateChanged()
{
	bRefreshPending = true;
}

void UjinzzaBoomboxWidget::RefreshFromBoombox()
{
	AjinzzaBoomboxProp* Prop = Boombox.Get();
	if (!Prop || !TrackListBox)
	{
		return;
	}

	const FJinzzaBoomboxMusicState& State = Prop->GetMusicState();
	const TArray<FJinzzaBoomboxTrack>& Playlist = Prop->GetPlaylist();

	if (StatusText)
	{
		FString Status = TEXT("Nothing playing");
		if (State.HasSource())
		{
			const FString SourceName = State.Url.IsEmpty()
				? (Playlist.IsValidIndex(State.TrackIndex) ? Playlist[State.TrackIndex].DisplayName.ToString() : FString(TEXT("?")))
				: State.Url;
			Status = FString::Printf(TEXT("%s: %s"), State.bPlaying ? TEXT("Now playing") : TEXT("Paused"), *SourceName);
		}
		StatusText->SetText(FText::FromString(Status));
	}

	if (PlayPauseLabel)
	{
		PlayPauseLabel->SetText(FText::FromString(State.HasSource() && State.bPlaying ? TEXT("Pause") : TEXT("Play")));
	}

	TrackListBox->ClearChildren();
	for (int32 Index = 0; Index < Playlist.Num(); ++Index)
	{
		const bool bIsCurrent = State.Url.IsEmpty() && State.TrackIndex == Index;
		const FString Prefix = bIsCurrent ? (State.bPlaying ? TEXT("[Playing]  ") : TEXT("[Paused]  ")) : TEXT("");
		const FText Label = FText::FromString(Prefix + Playlist[Index].DisplayName.ToString());
		const FName RowName = *FString::Printf(TEXT("TrackRow_%d"), NextRowId++);

		// The current song's row is a filled pink sticker, the rest plain black ones.
		UButton* RowButton = JinzzaUI::MakeStickerButton(WidgetTree, RowName, Label, JinzzaUI::Sticker_Pink, 20.f, bIsCurrent);

		UJinzzaBoomboxRowHandler* Handler = NewObject<UJinzzaBoomboxRowHandler>(this);
		Handler->TrackIndex = Index;
		Handler->OwnerWidget = this;
		RowButton->OnClicked.AddDynamic(Handler, &UJinzzaBoomboxRowHandler::HandleClicked);

		if (UVerticalBoxSlot* RowSlot = TrackListBox->AddChildToVerticalBox(RowButton))
		{
			RowSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 8.f));
			RowSlot->SetHorizontalAlignment(HAlign_Fill);
		}
	}
}

void UjinzzaBoomboxWidget::OnTrackClicked(int32 TrackIndex)
{
	AjinzzaBoomboxProp* Prop = Boombox.Get();
	AjinzzaCharacter* Character = GetOwningPlayerPawn<AjinzzaCharacter>();
	if (!Prop || !Character)
	{
		return;
	}

	const FJinzzaBoomboxMusicState& State = Prop->GetMusicState();
	const bool bIsCurrent = State.Url.IsEmpty() && State.TrackIndex == TrackIndex;

	// Click a different song: play it from the start. Click the current one: pause / resume it.
	Character->RequestBoomboxMusic(Prop, TrackIndex, FString(), bIsCurrent ? !State.bPlaying : true);
}

void UjinzzaBoomboxWidget::SetLinkMessage(const FString& Message, bool bError)
{
	if (LinkMessageText)
	{
		LinkMessageText->SetText(FText::FromString(Message));
		LinkMessageText->SetColorAndOpacity(FSlateColor(bError ? JinzzaUI::Sticker_Coral : JinzzaUI::Sticker_SubText));
	}
}

void UjinzzaBoomboxWidget::SubmitLink()
{
	AjinzzaBoomboxProp* Prop = Boombox.Get();
	AjinzzaCharacter* Character = GetOwningPlayerPawn<AjinzzaCharacter>();
	if (!Prop || !Character || !LinkTextBox)
	{
		return;
	}

	const FString Url = LinkTextBox->GetText().ToString().TrimStartAndEnd();
	if (!AjinzzaBoomboxProp::IsAcceptableMusicUrl(Url))
	{
		SetLinkMessage(TEXT("That doesn't look like a link. It has to start with http:// or https:// and have no spaces."), true);
		return;
	}

	SetLinkMessage(TEXT("Sent. If nothing plays, the link isn't a direct audio file (YouTube / Spotify page links can't be played)."), false);
	Character->RequestBoomboxMusic(Prop, -1, Url, true);
}

void UjinzzaBoomboxWidget::OnPlayLinkClicked()
{
	SubmitLink();
}

void UjinzzaBoomboxWidget::OnLinkCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		SubmitLink();
	}
}

void UjinzzaBoomboxWidget::OnPlayPauseClicked()
{
	AjinzzaBoomboxProp* Prop = Boombox.Get();
	AjinzzaCharacter* Character = GetOwningPlayerPawn<AjinzzaCharacter>();
	if (!Prop || !Character)
	{
		return;
	}

	const FJinzzaBoomboxMusicState& State = Prop->GetMusicState();
	if (State.HasSource())
	{
		Character->RequestBoomboxMusic(Prop, State.TrackIndex, State.Url, !State.bPlaying);
	}
	else if (Prop->GetPlaylist().Num() > 0)
	{
		Character->RequestBoomboxMusic(Prop, 0, FString(), true);
	}
}

void UjinzzaBoomboxWidget::OnCloseClicked()
{
	if (AjinzzaCharacter* Character = GetOwningPlayerPawn<AjinzzaCharacter>())
	{
		Character->CloseHeldPropUI();
	}
	else
	{
		RemoveFromParent();
	}
}

FReply UjinzzaBoomboxWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	return FReply::Handled();
}

FReply UjinzzaBoomboxWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnCloseClicked();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}
