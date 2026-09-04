// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaEmoteWheelWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"

void UjinzzaEmoteWheelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	float MouseX = 0.f, MouseY = 0.f;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	const FVector2D Center = ViewportSize * 0.5f;
	const FVector2D Delta = FVector2D(MouseX, MouseY) - Center;

	EJinzzaEmoteType NewHovered = EJinzzaEmoteType::None;

	// Dead zone at screen center so resting the mouse there doesn't commit to a direction.
	constexpr float DeadZoneSq = 20.f * 20.f;
	if (Delta.SizeSquared() >= DeadZoneSq)
	{
		if (FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y))
		{
			NewHovered = Delta.X >= 0.f ? EJinzzaEmoteType::Point : EJinzzaEmoteType::MiddleFinger;
		}
		else
		{
			// Screen space Y grows downward, so a negative Y delta is "up".
			NewHovered = Delta.Y < 0.f ? EJinzzaEmoteType::ThumbsUp : EJinzzaEmoteType::ThumbsDown;
		}
	}

	if (NewHovered != HoveredEmote)
	{
		HoveredEmote = NewHovered;
		BP_OnHoveredEmoteChanged(HoveredEmote);
	}
}
