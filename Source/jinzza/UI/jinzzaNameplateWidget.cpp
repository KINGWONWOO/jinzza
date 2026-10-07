// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaNameplateWidget.h"
#include "jinzzaPartyPlayerState.h"
#include "jinzzaGameGameState.h"
#include "jinzzaUIStyle.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	/** Past this (cm) a name tag isn't drawn - screen-space tags would otherwise show through walls across the map. */
	constexpr float NameplateMaxDistance = 2500.f;
}

void UjinzzaNameplateWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	// Small black sticker pill with white text, like the interaction prompt.
	UBorder* Face = nullptr;
	UOverlay* Root = JinzzaUI::MakeSticker(WidgetTree, TEXT("Root"), JinzzaUI::Sticker_Ink, 16.f, Face, 3.f, 3.f);
	WidgetTree->RootWidget = Root;
	Face->SetPadding(FMargin(12.f, 4.f));
	Face->SetHorizontalAlignment(HAlign_Center);

	NameText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("NameText"), FText::GetEmpty(), 18);
	Face->SetContent(NameText);
}

void UjinzzaNameplateWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UjinzzaNameplateWidget::Refresh()
{
	APawn* Pawn = OwnerPawn.Get();
	const UWorld* World = GetWorld();
	if (!Pawn || !World || Pawn->IsLocallyControlled())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FString Name;
	if (const APlayerState* PS = Pawn->GetPlayerState())
	{
		const AjinzzaPartyPlayerState* PartyPS = Cast<AjinzzaPartyPlayerState>(PS);
		// In the match, only ever show aliases.
		const bool bInMatch = World->GetGameState<AjinzzaGameGameState>() != nullptr;
		if (!bInMatch || (PartyPS && PartyPS->HasDisplayAlias()))
		{
			Name = AjinzzaPartyPlayerState::GetDisplayNameFor(PS);
		}
	}

	bool bInRange = true;
	if (const APlayerController* LocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(World) : nullptr)
	{
		if (LocalPC->PlayerCameraManager)
		{
			bInRange = FVector::DistSquared(LocalPC->PlayerCameraManager->GetCameraLocation(), Pawn->GetActorLocation()) <= FMath::Square(NameplateMaxDistance);
		}
	}

	if (Name.IsEmpty() || !bInRange)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (Name != ShownName && NameText)
	{
		ShownName = Name;
		NameText->SetText(FText::FromString(Name));
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
