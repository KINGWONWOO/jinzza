// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaLobbyTimeOfDay.generated.h"

/** Shared lobby time-of-day preset, cycled by AjinzzaLobbyClock and applied to the sun/sky by AjinzzaLobbyGameState. */
UENUM(BlueprintType)
enum class EJinzzaLobbyTimeOfDay : uint8
{
	Day,
	Sunset,
	Night
};
