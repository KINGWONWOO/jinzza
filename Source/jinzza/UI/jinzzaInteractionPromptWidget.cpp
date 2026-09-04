// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaInteractionPromptWidget.h"
#include "Components/TextBlock.h"

void UjinzzaInteractionPromptWidget::SetPrompt(const FText& Text)
{
	if (PromptText)
	{
		PromptText->SetText(Text);
	}
}
