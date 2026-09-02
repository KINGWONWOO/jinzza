// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaMatchSettings.generated.h"

/** Configurable pre-match settings, set by the host in the Host Setup panel and replicated via AjinzzaLobbyGameState. */
USTRUCT(BlueprintType)
struct FJinzzaMatchSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RoomName = TEXT("JINZZA Room");

	/** 4-12 per the design doc's lobby player-count range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxPlayers = 6;

	/** 1-2 (2-judge mode is a stretch option per the design doc). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 JudgeCount = 1;

	/** 1-3. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 VoteCount = 1;

	/** One of "Slow" / "Normal" / "Fast". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PhaseSpeed = TEXT("Normal");

	/** One of "Random" / "Host Picks". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RoleAssignMethod = TEXT("Random");
};
