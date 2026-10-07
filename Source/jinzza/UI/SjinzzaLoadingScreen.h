// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Fonts/SlateFontInfo.h"

struct FSlateBrush;

/**
 * Full-screen loading screen: logo, spinner + status line, progress bar, and a random tip.
 *
 * Plain Slate (not UMG) on purpose: the same widget class is shown by the MoviePlayer on its own
 * thread during a blocking map load, and added straight to the game viewport (which outlives worlds)
 * for seamless travel and the post-load preload - see UjinzzaLoadingScreenSubsystem. The MoviePlayer
 * copy is given fixed Status/Progress values; only the viewport copy binds them to the subsystem.
 */
class JINZZA_API SjinzzaLoadingScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SjinzzaLoadingScreen)
		: _LogoBrush(nullptr)
	{}
		/** May be null - a text title is drawn instead. Must outlive the widget. */
		SLATE_ARGUMENT(const FSlateBrush*, LogoBrush)
		SLATE_ARGUMENT(FText, Tip)
		SLATE_ARGUMENT(FSlateFontInfo, TitleFont)
		SLATE_ARGUMENT(FSlateFontInfo, BodyFont)
		SLATE_ATTRIBUTE(FText, StatusText)
		/** Unset = indeterminate (animated) bar. */
		SLATE_ATTRIBUTE(TOptional<float>, Progress)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};
