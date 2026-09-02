// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGameEndWidget.h"
#include "jinzzaUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "jinzzaGameInstance.h"

void UjinzzaGameEndWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("GameEndRootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Panel = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("GameEndPanel"));
	Panel->SetHorizontalAlignment(HAlign_Left);
	Panel->SetVerticalAlignment(VAlign_Bottom);

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
	PanelSlot->SetAlignment(FVector2D(0.f, 1.f));
	PanelSlot->SetPosition(FVector2D(24.f, -24.f));
	PanelSlot->SetAutoSize(true);

	EndGameButton = JinzzaUI::MakeWarningButton(WidgetTree, TEXT("EndGameButton"), FText::FromString(TEXT("End Game (return to Lobby)")), 18.f);
	EndGameButton->OnClicked.AddDynamic(this, &UjinzzaGameEndWidget::OnEndGameClicked);
	Panel->SetContent(EndGameButton);

	const APlayerController* PC = GetOwningPlayer();
	EndGameButton->SetVisibility(PC && PC->HasAuthority() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UjinzzaGameEndWidget::OnEndGameClicked()
{
	if (UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->EndGameReturnToLobby();
	}
}
