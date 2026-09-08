// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaWorldSignWidget.h"
#include "Components/TextBlock.h"

void UjinzzaWorldSignWidget::SetSignText(const FText& Text)
{
	if (SignText)
	{
		SignText->SetText(Text);
	}
}
