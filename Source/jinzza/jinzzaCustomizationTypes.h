// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaCustomizationTypes.generated.h"

/**
 * A few temporary placeholder options for a customization slot that doesn't have real content
 * yet (Head/Top/Eyebrows/Eyes - see UjinzzaGameUserSettings). Once real items exist for a slot,
 * replace this with whatever per-slot item list makes sense (e.g. a DataTable row reference) -
 * nothing else that reads these values needs to change shape, just the enum -> item lookup.
 */
UENUM(BlueprintType)
enum class EJinzzaCustomizationStyle : uint8
{
	StyleA,
	StyleB,
	StyleC
};

/** Placeholder hair color options - real enough to actually tint a swatch/material with, unlike the generic StyleA/B/C slots. */
UENUM(BlueprintType)
enum class EJinzzaHairColor : uint8
{
	Black,
	Brown,
	Blonde,
	Red,
	Blue
};
