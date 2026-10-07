// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DPICustomScalingRule.h"
#include "jinzzaUIScalingRule.generated.h"

/**
 * Project-wide UI scale rule (DefaultEngine.ini, [/Script/Engine.UserInterfaceSettings]
 * UIScaleRule=Custom). Every jinzza widget is laid out in fixed slate units for a 1920x1080 design
 * canvas (SizeBox widths, overlay paddings, font sizes). This rule picks the largest scale at which
 * that whole canvas still fits the viewport: min(Width / 1920, Height / 1080).
 *
 * Effect: at any resolution or aspect ratio the UI keeps its proportions relative to the screen, and
 * edge/center-aligned elements stay at the same relative spot. Wider-than-16:9 screens get extra
 * horizontal room (side-anchored elements move outward), narrower ones get extra vertical room -
 * nothing is ever cropped. The engine's default ShortestSide rule only looked at height, so on 4:3,
 * 16:10, or a narrow PIE window the wide panels (Settings 1240, Customization 1060) overflowed and
 * overlapped the side columns.
 */
UCLASS()
class JINZZA_API UjinzzaUIScalingRule : public UDPICustomScalingRule
{
	GENERATED_BODY()

public:
	static constexpr float DesignWidth = 1920.f;
	static constexpr float DesignHeight = 1080.f;

	virtual float GetDPIScaleBasedOnSize(FIntPoint Size) const override;
};
