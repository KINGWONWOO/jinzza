// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Limits for chat-board messages (AjinzzaPlayerController::Server_SendChatMessage). */
namespace JinzzaChat
{
	/** Also roughly what fits on the board at its text size. */
	inline constexpr int32 MaxMessageLength = 120;
	inline constexpr double MinSecondsBetweenMessages = 0.5;
}
