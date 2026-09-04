// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaEmoteTypes.generated.h"

/**
 * The four emote-wheel directions (design doc's Week 10 emote system). To add a new one: add a
 * case here, a matching UAnimMontage property + switch case on AjinzzaCharacter
 * (GetMontageForEmote), and a matching quadrant in UjinzzaEmoteWheelWidget.
 */
UENUM(BlueprintType)
enum class EJinzzaEmoteType : uint8
{
	None,
	ThumbsUp,
	ThumbsDown,
	MiddleFinger,
	Point
};
