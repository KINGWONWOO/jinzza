// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaChatTypes.generated.h"

/** One chat line, built by the server (AjinzzaPlayerController::Server_SendChatMessage) and sent to each recipient. */
USTRUCT(BlueprintType)
struct FJinzzaChatMessage
{
	GENERATED_BODY()

	/** The sender's display name at send time - their nickname, or in the match their alias ("User3", "Judge"). */
	UPROPERTY(BlueprintReadOnly)
	FString SenderName;

	UPROPERTY(BlueprintReadOnly)
	FString Text;

	/** Sent by a ghost - only other ghosts receive it. */
	UPROPERTY(BlueprintReadOnly)
	bool bFromGhost = false;
};

namespace JinzzaChat
{
	inline constexpr int32 MaxMessageLength = 120;
	inline constexpr double MinSecondsBetweenMessages = 0.5;
}
