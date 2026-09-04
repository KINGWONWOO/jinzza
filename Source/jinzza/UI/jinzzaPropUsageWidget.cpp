// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaPropUsageWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UjinzzaPropUsageWidget::SetPropInfo(UTexture2D* Icon, const FText& Description)
{
	if (UsageIcon)
	{
		UsageIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Icon)
		{
			UsageIcon->SetBrushFromTexture(Icon);
		}
	}

	if (UsageText)
	{
		UsageText->SetText(Description);
	}
}
