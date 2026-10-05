// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaFriendInviteWidget.h"
#include "jinzzaUIStyle.h"
#include "jinzzaGameInstance.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"

void UJinzzaFriendInviteRowHandler::HandleInviteClicked()
{
	if (UjinzzaFriendInviteWidget* Owner = OwnerWidget.Get())
	{
		Owner->InviteFriend(NetIdString);
	}
}

void UjinzzaFriendInviteWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

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

	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeStickerHeading(WidgetTree, TEXT("Title"), FText::FromString(TEXT("Invite Friends")), 48), 0.f);

	HeaderNote = JinzzaUI::MakeStickerText(WidgetTree, TEXT("HeaderNote"), FText::FromString(TEXT("Loading friends list...")), 20, true);
	JinzzaUI::AddSpaced(Stack, HeaderNote, 12.f);

	FriendListScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("FriendListScrollBox"));
	USizeBox* ScrollHeightBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FriendListHeightBox"));
	ScrollHeightBox->SetHeightOverride(400.f);
	ScrollHeightBox->AddChild(FriendListScrollBox);
	JinzzaUI::AddSpaced(Stack, ScrollHeightBox, 10.f);

	FriendListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FriendListBox"));
	FriendListScrollBox->AddChild(FriendListBox);

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

	RefreshButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("RefreshButton"), FText::FromString(TEXT("Refresh")), JinzzaUI::Sticker_Yellow, 26.f, true);
	ButtonRow->AddChildToHorizontalBox(RefreshButton);
}

void UjinzzaFriendInviteWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();

	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddDynamic(this, &UjinzzaFriendInviteWidget::OnRefreshClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UjinzzaFriendInviteWidget::OnCloseClicked);
	}

	if (UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		FriendsListReceivedHandle = GI->OnFriendsListReceived.AddUObject(this, &UjinzzaFriendInviteWidget::HandleFriendsListReceived);
	}

	RequestFriends();
}

void UjinzzaFriendInviteWidget::NativeDestruct()
{
	if (UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->OnFriendsListReceived.Remove(FriendsListReceivedHandle);
	}

	Super::NativeDestruct();
}

void UjinzzaFriendInviteWidget::RequestFriends()
{
	if (HeaderNote)
	{
		HeaderNote->SetText(FText::FromString(TEXT("Loading friends list...")));
	}

	if (UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->RequestFriendsList();
	}
}

void UjinzzaFriendInviteWidget::HandleFriendsListReceived(const TArray<FJinzzaFriendInfo>& Friends)
{
	if (!FriendListBox)
	{
		return;
	}

	FriendListBox->ClearChildren();

	if (Friends.Num() == 0)
	{
		if (HeaderNote)
		{
			HeaderNote->SetText(FText::FromString(TEXT("No Steam friends found.")));
		}
		return;
	}

	if (HeaderNote)
	{
		HeaderNote->SetText(FText::FromString(FString::Printf(TEXT("%d friend(s)"), Friends.Num())));
	}

	for (const FJinzzaFriendInfo& Friend : Friends)
	{
		const FName RowName = *FString::Printf(TEXT("FriendRow_%d"), NextRowId++);

		UButton* InviteButton = JinzzaUI::MakeStickerButton(WidgetTree, *(RowName.ToString() + TEXT("_Invite")), FText::FromString(TEXT("Invite")), JinzzaUI::Sticker_Teal, 20.f);
		InviteButton->SetIsEnabled(Friend.bIsOnline);

		UJinzzaFriendInviteRowHandler* Handler = NewObject<UJinzzaFriendInviteRowHandler>(this);
		Handler->NetIdString = Friend.NetIdString;
		Handler->OwnerWidget = this;
		InviteButton->OnClicked.AddDynamic(Handler, &UJinzzaFriendInviteRowHandler::HandleInviteClicked);

		const FString RowLabel = Friend.bIsOnline ? Friend.DisplayName : FString::Printf(TEXT("%s (Offline)"), *Friend.DisplayName);
		// Big name on the left, Invite pill on the right (same row helper as Settings).
		JinzzaUI::AddStickerRow(WidgetTree, FriendListBox, RowName, FText::FromString(RowLabel), InviteButton, 150.f);
	}
}

void UjinzzaFriendInviteWidget::InviteFriend(const FString& NetIdString)
{
	if (UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->InviteFriendToSession(NetIdString);
	}
}

void UjinzzaFriendInviteWidget::OnRefreshClicked()
{
	RequestFriends();
}

void UjinzzaFriendInviteWidget::OnCloseClicked()
{
	RemoveFromParent();
}
