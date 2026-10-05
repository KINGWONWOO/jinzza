// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGameEndWidget.h"
#include "jinzzaUIStyle.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "jinzzaGameInstance.h"

void UjinzzaGameEndWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Filled coral sticker pill (the "stop/leave" color), matching the menus / T_Logo.
	EndGameButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("EndGameButton"), FText::FromString(TEXT("End Game")), JinzzaUI::Sticker_Coral, 28.f, true);
	if (UOverlaySlot* ButtonSlot = Root->AddChildToOverlay(EndGameButton))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonSlot->SetVerticalAlignment(VAlign_Bottom);
		ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 40.f));
	}
}

void UjinzzaGameEndWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();

	if (EndGameButton)
	{
		EndGameButton->OnClicked.AddDynamic(this, &UjinzzaGameEndWidget::OnEndGameClicked);

		const APlayerController* PC = GetOwningPlayer();
		EndGameButton->SetVisibility(PC && PC->HasAuthority() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UjinzzaGameEndWidget::OnEndGameClicked()
{
	if (UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->EndGameReturnToLobby();
	}
}
