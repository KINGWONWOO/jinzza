// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaRoundTypes.generated.h"

/** The 8-phase round structure from the design doc's section 6, plus a terminal state. */
UENUM(BlueprintType)
enum class EJinzzaRoundPhase : uint8
{
	None,
	RoleAssignment,
	SelfIntroduction,
	FreeTime1,
	QuestionTime,
	MidEvaluation,
	FreeTime2,
	Interview,
	FinalDecision,
	RoundComplete
};

/** A player's secret role for the round. Deliberately never broadcast-replicated - see AjinzzaPartyPlayerState. */
UENUM(BlueprintType)
enum class EJinzzaPartyRole : uint8
{
	None,
	RealOne,
	Imitator,
	Judge
};

/** Disguise face material variant (design doc section 12/13-2: MI_Face_A/B/C). */
UENUM(BlueprintType)
enum class EJinzzaFaceType : uint8
{
	None,
	A,
	B,
	C
};

/** Voice filter preset (design doc section 13-5: SC_VoiceFilter_High/Low/Robot). Not yet consumed - Week 6 (Vivox). */
UENUM(BlueprintType)
enum class EJinzzaVoiceFilter : uint8
{
	None,
	High,
	Low,
	Robot
};
