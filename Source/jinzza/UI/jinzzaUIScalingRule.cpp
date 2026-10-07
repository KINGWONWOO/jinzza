// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaUIScalingRule.h"

float UjinzzaUIScalingRule::GetDPIScaleBasedOnSize(FIntPoint Size) const
{
	if (Size.X <= 0 || Size.Y <= 0)
	{
		return 1.f;
	}

	const float Scale = FMath::Min(Size.X / DesignWidth, Size.Y / DesignHeight);

	// Floor keeps text from collapsing to nothing in a tiny editor/PIE window.
	return FMath::Max(Scale, 0.25f);
}
